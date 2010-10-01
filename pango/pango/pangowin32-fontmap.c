/* Pango
 * pangowin32-fontmap.c: Win32 font handling
 *
 * Copyright (C) 2000 Red Hat Software
 * Copyright (C) 2000 Tor Lillqvist
 * Copyright (C) 2001 Alexander Larsson
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "pango-fontmap.h"
#include "pango-utils.h"
#include "pangowin32-private.h"

#define PANGO_TYPE_WIN32_FONT_MAP           (pango_win32_font_map_get_type ())
#define PANGO_WIN32_FONT_MAP(object)        (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_WIN32_FONT_MAP, PangoWin32FontMap))
#define PANGO_WIN32_IS_FONT_MAP(object)     (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_WIN32_FONT_MAP))

typedef struct _PangoWin32FontMap      PangoWin32FontMap;
typedef struct _PangoWin32Family       PangoWin32Family;
typedef struct _PangoWin32SizeInfo     PangoWin32SizeInfo;

/* Number of freed fonts */
#define MAX_FREED_FONTS 16

struct _PangoWin32FontMap
{
  PangoFontMap parent_instance;

  PangoWin32FontCache *font_cache;
  GQueue *freed_fonts;

  /* Map Pango family names to PangoWin32Family structs */
  GHashTable *families;

  /* Map LOGFONTS (taking into account only the lfFaceName, lfItalic
   * and lfWeight fields) to PangoWin32SizeInfo structs.
   */
  GHashTable *size_infos;

  int n_fonts;

  double resolution;		/* (points / pixel) * PANGO_SCALE */
};

struct _PangoWin32FontMapClass
{
  PangoFontMapClass parent_class;
};

struct _PangoWin32Family
{
  PangoFontFamily parent_instance;

  char *family_name;
  GSList *font_entries;
  BYTE pitch_and_family;
};

struct _PangoWin32SizeInfo
{
  GSList *logfonts;
};

#define PANGO_WIN32_TYPE_FAMILY              (pango_win32_family_get_type ())
#define PANGO_WIN32_FAMILY(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_WIN32_TYPE_FAMILY, PangoWin32Family))
#define PANGO_WIN32_IS_FAMILY(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_WIN32_TYPE_FAMILY))

#define PANGO_WIN32_TYPE_FACE              (pango_win32_face_get_type ())
#define PANGO_WIN32_FACE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_WIN32_TYPE_FACE, PangoWin32Face))
#define PANGO_WIN32_IS_FACE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_WIN32_TYPE_FACE))

GType             pango_win32_family_get_type        (void);
GType             pango_win32_face_get_type          (void);

static void       pango_win32_font_map_init          (PangoWin32FontMap       *fontmap);
static void       pango_win32_font_map_class_init    (PangoFontMapClass       *class);

static void       pango_win32_font_map_finalize      (GObject                      *object);
static PangoFont *pango_win32_font_map_load_font     (PangoFontMap                 *fontmap,
						      PangoContext                 *context,
						      const PangoFontDescription   *description);
static void       pango_win32_font_map_list_families (PangoFontMap                 *fontmap,
						       PangoFontFamily            ***families,
						       int                          *n_families);
static PangoFontset *pango_win32_font_map_load_fontset (PangoFontMap               *fontmap,
							PangoContext               *context,
							const PangoFontDescription *desc,
							PangoLanguage              *language);  
static void       pango_win32_fontmap_cache_clear    (PangoWin32FontMap            *win32fontmap);

static void       pango_win32_insert_font            (PangoWin32FontMap            *fontmap,
						      LOGFONT                      *lfp);

static PangoFontClass *font_map_parent_class;	/* Parent class structure for PangoWin32FontMap */

static GType
pango_win32_font_map_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (PangoFontMapClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) pango_win32_font_map_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (PangoWin32FontMap),
        0,              /* n_preallocs */
        (GInstanceInitFunc) pango_win32_font_map_init,
      };
      
      object_type = g_type_register_static (PANGO_TYPE_FONT_MAP,
                                            "PangoWin32FontMap",
                                            &object_info, 0);
    }
  
  return object_type;
}

/* A hash function for LOGFONTs that takes into consideration only
 * those fields that indicate a specific .ttf file is in use:
 * lfFaceName, lfItalic and lfWeight. Dunno how correct this is.
 */

static guint
logfont_nosize_hash (gconstpointer v)
{
  const LOGFONT *lfp = v;
  gchar *face = g_ascii_strdown (lfp->lfFaceName, -1);
  guint result = g_str_hash (face) + lfp->lfItalic + lfp->lfWeight;

  g_free (face);

  return result;
}

/* Ditto comparison function */
static gboolean
logfont_nosize_equal (gconstpointer v1,
		      gconstpointer v2)
{
  const LOGFONT *lfp1 = v1, *lfp2 = v2;

  return (g_ascii_strcasecmp (lfp1->lfFaceName, lfp2->lfFaceName) == 0
	  && lfp1->lfItalic == lfp2->lfItalic
	  && lfp1->lfWeight == lfp2->lfWeight);
}
  
static void 
pango_win32_font_map_init (PangoWin32FontMap *win32fontmap)
{
  win32fontmap->families = g_hash_table_new (g_str_hash, g_str_equal);
  win32fontmap->size_infos =
    g_hash_table_new (logfont_nosize_hash, logfont_nosize_equal);
  win32fontmap->n_fonts = 0;
}

static void
pango_win32_font_map_class_init (PangoFontMapClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  
  font_map_parent_class = g_type_class_peek_parent (class);
  
  object_class->finalize = pango_win32_font_map_finalize;
  class->load_font = pango_win32_font_map_load_font;
  class->list_families = pango_win32_font_map_list_families;
  class->load_fontset = pango_win32_font_map_load_fontset;

  pango_win32_get_dc ();
}

static PangoWin32FontMap *fontmap = NULL;

static int CALLBACK
pango_win32_inner_enum_proc (LOGFONT    *lfp,
			     TEXTMETRIC *metrics,
			     DWORD       fontType,
			     LPARAM      lParam)
{
  /* Windows generates synthetic vertical writing versions of East
   * Asian fonts with @ prepended to their name, ignore them.
   */
  if (lfp->lfFaceName[0] != '@')
    pango_win32_insert_font (fontmap, lfp);

  return 1;
}

static int CALLBACK
pango_win32_enum_proc (LOGFONT    *lfp,
		       TEXTMETRIC *metrics,
		       DWORD       fontType,
		       LPARAM      lParam)
{
  LOGFONT lf;

  if (fontType != TRUETYPE_FONTTYPE)
    return 1;

  lf = *lfp;

  PING(("%s", lf.lfFaceName));
  EnumFontFamiliesExA (pango_win32_hdc, &lf,
		       (FONTENUMPROC) pango_win32_inner_enum_proc,
		       lParam, 0);

  return 1;
}

/**
 * pango_win32_font_map_for_display:
 *
 * Returns a #PangoWin32FontMap. Font maps are cached and should
 * not be freed. If the font map is no longer needed, it can
 * be released with pango_win32_shutdown_display().
 *
 * Returns: a #PangoFontMap.
 **/
PangoFontMap *
pango_win32_font_map_for_display (void)
{
  LOGFONT logfont;

  /* Make sure that the type system is initialized */
  g_type_init ();
  
  if (fontmap != NULL)
    return PANGO_FONT_MAP (fontmap);

  fontmap = g_object_new (PANGO_TYPE_WIN32_FONT_MAP, NULL);
  
  fontmap->font_cache = pango_win32_font_cache_new ();
  fontmap->freed_fonts = g_queue_new ();

  memset (&logfont, 0, sizeof (logfont));
  logfont.lfCharSet = DEFAULT_CHARSET;
  EnumFontFamiliesExA (pango_win32_hdc, &logfont, (FONTENUMPROC) pango_win32_enum_proc, 0, 0);

  fontmap->resolution = ((double)PANGO_SCALE / (double) GetDeviceCaps (pango_win32_hdc, LOGPIXELSY)) * 72.0;

  return PANGO_FONT_MAP (fontmap);
}

/**
 * pango_win32_shutdown_display:
 * 
 * Free cached resources.
 **/
void
pango_win32_shutdown_display (void)
{
  if (fontmap)
    {
      pango_win32_fontmap_cache_clear (fontmap);
      g_object_unref (fontmap);

      fontmap = NULL;
    }
}

static void
pango_win32_font_map_finalize (GObject *object)
{
  PangoWin32FontMap *win32fontmap = PANGO_WIN32_FONT_MAP (object);
  
  g_list_foreach (win32fontmap->freed_fonts->head, (GFunc)g_object_unref, NULL);
  g_queue_free (win32fontmap->freed_fonts);
  
  pango_win32_font_cache_free (win32fontmap->font_cache);

  G_OBJECT_CLASS (font_map_parent_class)->finalize (object);
}

/*
 * PangoWin32Family
 */
static void
pango_win32_family_list_faces (PangoFontFamily  *family,
			       PangoFontFace  ***faces,
			       int              *n_faces)
{
  PangoWin32Family *win32family = PANGO_WIN32_FAMILY (family);
 
  *n_faces = g_slist_length (win32family->font_entries);
  if (faces)
    {
      GSList *tmp_list;
      int i = 0;
      
      *faces = g_new (PangoFontFace *, *n_faces);

      tmp_list = win32family->font_entries;
      while (tmp_list)
	{
	  (*faces)[i++] = tmp_list->data;
	  tmp_list = tmp_list->next;
	}
    }
}

const char *
pango_win32_family_get_name (PangoFontFamily  *family)
{
  PangoWin32Family *win32family = PANGO_WIN32_FAMILY (family);
  return win32family->family_name;
}

static void
pango_win32_family_class_init (PangoFontFamilyClass *class)
{
  class->list_faces = pango_win32_family_list_faces;
  class->get_name = pango_win32_family_get_name;
}

GType
pango_win32_family_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (PangoFontFamilyClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) pango_win32_family_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (PangoWin32Family),
        0,              /* n_preallocs */
        (GInstanceInitFunc) NULL,
      };
      
      object_type = g_type_register_static (PANGO_TYPE_FONT_FAMILY,
                                            "PangoWin32Family",
                                            &object_info, 0);
    }
  
  return object_type;
}

static void
list_families_foreach (gpointer key,
		       gpointer value,
		       gpointer user_data)
{
  GSList **list = user_data;

  *list = g_slist_prepend (*list, value);
}

static void
pango_win32_font_map_list_families (PangoFontMap      *fontmap,
				    PangoFontFamily ***families,
				    int               *n_families)
{
  GSList *family_list = NULL;
  GSList *tmp_list;
  PangoWin32FontMap *win32fontmap = (PangoWin32FontMap *)fontmap;

  if (!n_families)
    return;

  g_hash_table_foreach (win32fontmap->families, list_families_foreach, &family_list);

  *n_families = g_slist_length (family_list);

  if (families)
    {
      int i = 0;
	
      *families = g_new (PangoFontFamily *, *n_families);

      tmp_list = family_list;
      while (tmp_list)
	{
	  (*families)[i] = tmp_list->data;
	  i++;
	  tmp_list = tmp_list->next;
	}
    }
  
  g_slist_free (family_list);
}

struct _PangoFontDescription
{
  char *family_name;
};

static PangoFontset *
pango_win32_font_map_load_fontset (PangoFontMap               *fontmap,
				   PangoContext               *context,
				   const PangoFontDescription *desc,
				   PangoLanguage              *language)
{
  const gchar *family = desc->family_name;
//  const gchar *family = pango_font_description_get_family (desc);

  if (family[0]=='s' && family[1]=='a' && family[2]=='n' && family[3]=='s' && family[4]==0)
	goto normal;

  if (family[0]=='m' && family[1]=='o' && family[2]=='n' && family[3]=='o' && family[4]=='s' &&
      family[5]=='p' && family[6]=='a' && family[7]=='c' && family[8]=='e' && family[9]==0)
//  if (strcmp (family, "monospace") == 0)
	goto normal;

  if (strchr (family, ','))
	goto normal;

    {
      PangoWin32FontMap *win32fontmap = (PangoWin32FontMap *) fontmap;
      PangoWin32Family *win32_family = g_hash_table_lookup (win32fontmap->families, family);

      if (win32_family != NULL)
	{
	  PangoFontset *retval;
	  PangoFontDescription *amended_desc = pango_font_description_copy_static (desc);
	  char *amended_family;
	  
	  if ((win32_family->pitch_and_family & 0xF0) == FF_ROMAN)
	    amended_family = g_strconcat (family, ",serif", NULL);
	  else if ((win32_family->pitch_and_family & 0xF0) == FF_MODERN)
	    amended_family = g_strconcat (family, ",monospace", NULL);
	  else
	    amended_family = g_strconcat (family, ",sans", NULL);
	  
	  pango_font_description_set_family_static (amended_desc, amended_family);
	  
	  retval = PANGO_FONT_MAP_CLASS (font_map_parent_class)->load_fontset (fontmap, context,
									       amended_desc, language);

	  pango_font_description_free (amended_desc);
	  g_free (amended_family);

	  return retval;
	}
    }

normal:
  return PANGO_FONT_MAP_CLASS (font_map_parent_class)->load_fontset
    (fontmap, context, desc, language);
}

static PangoWin32Family *
pango_win32_get_font_family (PangoWin32FontMap *win32fontmap,
			     const char        *family_name)
{
  PangoWin32Family *win32family = g_hash_table_lookup (win32fontmap->families, family_name);
  if (!win32family)
    {
      win32family = g_object_new (PANGO_WIN32_TYPE_FAMILY, NULL);
      win32family->family_name = g_strdup (family_name);
      win32family->font_entries = NULL;
      
      g_hash_table_insert (win32fontmap->families, win32family->family_name, win32family);
    }

  return win32family;
}

static PangoFont *
pango_win32_font_map_load_font (PangoFontMap               *fontmap,
				PangoContext               *context,
				const PangoFontDescription *description)
{
  PangoWin32FontMap *win32fontmap = (PangoWin32FontMap *)fontmap;
  PangoWin32Family *win32family;
  PangoFont *result = NULL;
  GSList *tmp_list;
  gchar *name;

  g_return_val_if_fail (description != NULL, NULL);
  
  name = g_ascii_strdown (pango_font_description_get_family (description), -1);

  PING(("name=%s", name));

  win32family = g_hash_table_lookup (win32fontmap->families, name);
  if (win32family)
    {
      PangoWin32Face *best_match = NULL;
      
      PING (("got win32family"));
      tmp_list = win32family->font_entries;
      while (tmp_list)
	{
	  PangoWin32Face *face = tmp_list->data;

	  if (pango_font_description_better_match (description,
						   best_match ? best_match->description : NULL,
						   face->description))
	    best_match = face;
	  
	  tmp_list = tmp_list->next;
	}

      if (best_match)
	{
	  GSList *tmp_list = best_match->cached_fonts;
	  gint size = pango_font_description_get_size (description);

	  PING(("got best match:%s size=%d",best_match->logfont.lfFaceName,size));

	  while (tmp_list)
	    {
	      PangoWin32Font *win32font = tmp_list->data;
	      if (win32font->size == size)
		{
		  PING (("size matches"));
		  result = (PangoFont *)win32font;

		  g_object_ref (result);
		  if (win32font->in_cache)
		    pango_win32_fontmap_cache_remove (fontmap, win32font);
		  
		  break;
		}
	      tmp_list = tmp_list->next;
	    }

	  if (!result)
	    {
	      PangoWin32Font *win32font = pango_win32_font_new (fontmap, &best_match->logfont, size);

	      win32font->fontmap = fontmap;
	      win32font->win32face = best_match;
	      best_match->cached_fonts = g_slist_prepend (best_match->cached_fonts, win32font);

	      result = (PangoFont *)win32font;
	    }
	}
      else
	PING(("no best match!"));
    }

  g_free (name);
  return result;
}

static gchar *
get_family_name (LOGFONT *lfp)
{
  HFONT hfont;
  HFONT oldhfont;

  struct name_header header;
  struct name_record record;

  gint unicode_ix = -1, mac_ix = -1, microsoft_ix = -1;
  gint name_ix;
  gchar *codeset;

  gchar *string = NULL;
  gchar *name;

  gint i, l;
  gsize nbytes;

  /* If lfFaceName is ASCII, assume it is the common (English) name
   * for the font. Is this valid? Do some TrueType fonts have
   * different names in French, German, etc, and does the system
   * return these if the locale is set to use French, German, etc?
   */
  l = strlen (lfp->lfFaceName);
  for (i = 0; i < l; i++)
    if (lfp->lfFaceName[i] < ' ' || lfp->lfFaceName[i] > '~')
      break;

  if (i == l)
    return g_strdup (lfp->lfFaceName);

  if ((hfont = CreateFontIndirect (lfp)) == NULL)
    goto fail0;
  
  if ((oldhfont = SelectObject (pango_win32_hdc, hfont)) == NULL)
    goto fail1;

  if (!pango_win32_get_name_header (pango_win32_hdc, &header))
    goto fail2;
  
  PING (("%d name records", header.num_records));

  for (i = 0; i < header.num_records; i++)
    {
      if (!pango_win32_get_name_record (pango_win32_hdc, i, &record))
	goto fail2;

      if ((record.name_id != 1 && record.name_id != 16) || record.string_length <= 0)
	continue;

      PING(("platform:%d encoding:%d language:%04x name_id:%d",
	    record.platform_id, record.encoding_id, record.language_id, record.name_id));
      
      if (record.platform_id == APPLE_UNICODE_PLATFORM_ID ||
	  record.platform_id == ISO_PLATFORM_ID)
	unicode_ix = i;
      else if (record.platform_id == MACINTOSH_PLATFORM_ID &&
	       record.encoding_id == 0 && /* Roman */
	       record.language_id == 0)	/* English */
	mac_ix = i;
      else if (record.platform_id == MICROSOFT_PLATFORM_ID)
	if ((microsoft_ix == -1 ||
	     PRIMARYLANGID (record.language_id) == LANG_ENGLISH) &&
	    (record.encoding_id == SYMBOL_ENCODING_ID ||
	     record.encoding_id == UNICODE_ENCODING_ID ||
	     record.encoding_id == UCS4_ENCODING_ID))
	  microsoft_ix = i;
    }

  if (microsoft_ix >= 0)
    name_ix = microsoft_ix;
  else if (mac_ix >= 0)
    name_ix = mac_ix;
  else if (unicode_ix >= 0)
    name_ix = unicode_ix;
  else
    goto fail2;
      
  if (!pango_win32_get_name_record (pango_win32_hdc, name_ix, &record))
    goto fail2;
  
  string = g_malloc (record.string_length + 1);
  if (GetFontData (pango_win32_hdc, NAME,
		   header.string_storage_offset + record.string_offset,
		   string, record.string_length) != record.string_length)
    goto fail2;
  
  string[record.string_length] = '\0';
  
  if (name_ix == microsoft_ix)
    if (record.encoding_id == SYMBOL_ENCODING_ID ||
	record.encoding_id == UNICODE_ENCODING_ID)
      codeset = "UTF-16BE";
    else
      codeset = "UCS-4BE";
  else if (name_ix == mac_ix)
    codeset = "MacRoman";
  else /* name_ix == unicode_ix */
    codeset = "UCS-4BE";
  
  name = g_convert (string, record.string_length, "UTF-8", codeset, NULL, &nbytes, NULL);
  if (name == NULL)
    goto fail2;
  g_free (string);

  PING(("%s", name));

  SelectObject (pango_win32_hdc, oldhfont);
  DeleteObject (hfont);

  return name;

 fail2:
  g_free (string);
  SelectObject (pango_win32_hdc, oldhfont);

 fail1:
  DeleteObject (hfont);
  
 fail0:
  return g_locale_to_utf8 (lfp->lfFaceName, -1, NULL, NULL, NULL);
}

static gchar *
get_family_name_lowercase (LOGFONT *lfp)
{
  gchar *family = get_family_name (lfp);
  gchar *lower = g_ascii_strdown (family, -1);

  g_free (family);
  return lower;
}

/* This inserts the given font into the size_infos table. If a SizeInfo
 * already exists with the same typeface name, then the font is added
 * to the SizeInfos list, else a new SizeInfo is created and inserted
 * in the table.
 */
static void
pango_win32_insert_font (PangoWin32FontMap *win32fontmap,
			 LOGFONT           *lfp)
{
  LOGFONT *lfp2 = NULL;
  PangoFontDescription *description;
  gchar *family_name;
  PangoWin32Family *font_family;
  PangoWin32Face *win32face;
  PangoWin32SizeInfo *size_info;
  PangoStyle style;
  PangoVariant variant;
  PangoWeight weight;
  PangoStretch stretch;
  GSList *tmp_list;
  gint i;

  PING(("face=%s,charset=%d,it=%d,wt=%ld,ht=%ld",lfp->lfFaceName,lfp->lfCharSet,lfp->lfItalic,lfp->lfWeight,lfp->lfHeight));
  
  /* Ignore Symbol fonts (which don't have any Unicode mapping
   * table). We could also be fancy and use the PostScript glyph name
   * table for such if present, and build a Unicode map by mapping
   * each PostScript glyph name to Unicode character. Oh well.
   */
  if (lfp->lfCharSet == SYMBOL_CHARSET)
    return;

  /* First insert the LOGFONT into the list of LOGFONTs for the typeface name
   */
  size_info = g_hash_table_lookup (win32fontmap->size_infos, lfp);
  if (!size_info)
    {
      PING(("SizeInfo not found"));
      size_info = g_new (PangoWin32SizeInfo, 1);
      size_info->logfonts = NULL;

      lfp2 = g_new (LOGFONT, 1);
      *lfp2 = *lfp;
      g_hash_table_insert (win32fontmap->size_infos, lfp2, size_info);
    }
  else
    {
      /* Don't store logfonts that differ only in charset
       */
      tmp_list = size_info->logfonts;
      while (tmp_list)
	{
	  LOGFONT *rover = tmp_list->data;
	  
	  /* We know that lfWeight, lfItalic and lfFaceName match.  We
	   * don't check lfHeight and lfWidth, those are used
	   * when creating a font.
	   */
	  if (rover->lfEscapement == lfp->lfEscapement &&
	      rover->lfOrientation == lfp->lfOrientation &&
	      rover->lfUnderline == lfp->lfUnderline &&
	      rover->lfStrikeOut == lfp->lfStrikeOut)
	    return;
	  
	  tmp_list = tmp_list->next;
	}
    }

  if (lfp2 == NULL)
    {
      lfp2 = g_new (LOGFONT, 1);
      *lfp2 = *lfp;
    }

  size_info->logfonts = g_slist_prepend (size_info->logfonts, lfp2);
  
  PING(("g_slist_length(size_info->logfonts)=%d", g_slist_length(size_info->logfonts)));

  family_name = get_family_name_lowercase (lfp);

  /* Convert the LOGFONT into a PangoFontDescription
   */
  if (!lfp2->lfItalic)
    style = PANGO_STYLE_NORMAL;
  else
    style = PANGO_STYLE_ITALIC;

  variant = PANGO_VARIANT_NORMAL;
  
  /* The PangoWeight values PANGO_WEIGHT_* map exactly do Windows FW_*
   * values.  Is this on purpose? Quantize the weight to exact
   * PANGO_WEIGHT_* values. Is this a good idea?
   */
  if (lfp2->lfWeight == FW_DONTCARE)
    weight = PANGO_WEIGHT_NORMAL;
  else if (lfp2->lfWeight <= (FW_ULTRALIGHT + FW_LIGHT) / 2)
    weight = PANGO_WEIGHT_ULTRALIGHT;
  else if (lfp2->lfWeight <= (FW_LIGHT + FW_NORMAL) / 2)
    weight = PANGO_WEIGHT_LIGHT;
  else if (lfp2->lfWeight <= (FW_NORMAL + FW_BOLD) / 2)
    weight = PANGO_WEIGHT_NORMAL;
  else if (lfp2->lfWeight <= (FW_BOLD + FW_ULTRABOLD) / 2)
    weight = PANGO_WEIGHT_BOLD;
  else if (lfp2->lfWeight <= (FW_ULTRABOLD + FW_HEAVY) / 2)
    weight = PANGO_WEIGHT_ULTRABOLD;
  else
    weight = PANGO_WEIGHT_HEAVY;

  /* XXX No idea how to figure out the stretch */
  stretch = PANGO_STRETCH_NORMAL;

  font_family = pango_win32_get_font_family (win32fontmap, family_name);
  g_free (family_name);

  if (font_family->font_entries == NULL)
    {
      font_family->pitch_and_family = lfp->lfPitchAndFamily;
/*      if ((font_family->pitch_and_family & 0xF0) == FF_MODERN)
	font_family->is_monospace = TRUE;*/
    }

  tmp_list = font_family->font_entries;
  while (tmp_list)
    {
      win32face = tmp_list->data;

      if (pango_font_description_get_style (win32face->description) == style &&
	  pango_font_description_get_weight (win32face->description) == weight &&
	  pango_font_description_get_stretch (win32face->description) == stretch &&
	  pango_font_description_get_variant (win32face->description) == variant)
	{
	  PING(("returning early"));
	  return;
	}

      tmp_list = tmp_list->next;
    }

  description = pango_font_description_new ();
  pango_font_description_set_family_static (description, font_family->family_name);
  pango_font_description_set_style (description, style);
  pango_font_description_set_weight (description, weight);
  pango_font_description_set_stretch (description, stretch);
  pango_font_description_set_variant (description, variant);

  win32face = g_object_new (PANGO_WIN32_TYPE_FACE, NULL);
  win32face->description = description;
  win32face->cached_fonts = NULL;
  
  for (i = 0; i < PANGO_WIN32_N_COVERAGES; i++)
     win32face->coverages[i] = NULL;
  win32face->logfont = *lfp;
  win32face->unicode_table = NULL;
  font_family->font_entries = g_slist_append (font_family->font_entries, win32face);
  win32fontmap->n_fonts++;

#if 1 /* Thought pango.aliases would make this code unnecessary, but no. */
  /*
   * There are magic family names coming from the X implementation.
   * They can be simply mapped to lfPitchAndFamily flag of the logfont
   * struct. These additional entries should probably only be references
   * to the respective entry created above. Thy are simply using the
   * same entry at the moment and it isn't crashing on g_free () ???
   * Maybe a memory leak ...
   */
  switch (lfp->lfPitchAndFamily & 0xF0)
    { 
    case FF_MODERN : /* monospace */
      PING(("monospace"));
      font_family = pango_win32_get_font_family (win32fontmap, "monospace");
      font_family->font_entries = g_slist_append (font_family->font_entries, win32face);
      win32fontmap->n_fonts++;
      break;
    case FF_ROMAN : /* serif */
      PING(("serif"));
      font_family = pango_win32_get_font_family (win32fontmap, "serif");
      font_family->font_entries = g_slist_append (font_family->font_entries, win32face);
      win32fontmap->n_fonts++;
      break;
    case  FF_SWISS  : /* sans */
      PING(("sans"));
      font_family = pango_win32_get_font_family (win32fontmap, "sans");
      font_family->font_entries = g_slist_append (font_family->font_entries, win32face);
      win32fontmap->n_fonts++;
      break;
    }

  /* Some other magic names */

  /* Recognize just "courier" for "courier new" */
  if (g_ascii_strcasecmp (win32face->logfont.lfFaceName, "courier new") == 0)
    {
      font_family = pango_win32_get_font_family (win32fontmap, "courier");
      font_family->font_entries = g_slist_append (font_family->font_entries, win32face);
      win32fontmap->n_fonts++;
    }
#endif
}

/* Given a LOGFONT and size, make a matching LOGFONT corresponding to
 * an installed font.
 */
void
pango_win32_make_matching_logfont (PangoFontMap  *fontmap,
				   const LOGFONT *lfp,
				   int            size,
				   LOGFONT       *out)
{
  PangoWin32FontMap *win32fontmap;
  GSList *tmp_list;
  PangoWin32SizeInfo *size_info;
  LOGFONT *closest_match = NULL;
  gint match_distance = 0;

  PING(("lfp.face=%s,wt=%ld,ht=%ld,size:%d",lfp->lfFaceName,lfp->lfWeight,lfp->lfHeight,size));
  win32fontmap = PANGO_WIN32_FONT_MAP (fontmap);
  
  size_info = g_hash_table_lookup (win32fontmap->size_infos, lfp);

  if (!size_info)
    {
      PING(("SizeInfo not found"));
      return;
    }

  tmp_list = size_info->logfonts;
  while (tmp_list)
    {
      LOGFONT *tmp_logfont = tmp_list->data;
      int font_size = abs (tmp_logfont->lfHeight);

      if (size != -1)
	{
	  int new_distance = (font_size == 0) ? 0 : abs (font_size - size);
	  
	  if (!closest_match ||
	      new_distance < match_distance ||
	      (new_distance < PANGO_SCALE && font_size != 0))
	    {
	      closest_match = tmp_logfont;
	      match_distance = new_distance;
	    }
	}

      tmp_list = tmp_list->next;
    }

  if (closest_match)
    {
      /* OK, we have a match; let's modify it to fit this size */

      *out = *closest_match;
      out->lfHeight = -(int)((double)size / win32fontmap->resolution + 0.5);
      out->lfWidth = 0;
    }
  else
    *out = *lfp; /* Whatever. We need to pass something... */
}

gint
pango_win32_coverage_language_classify (PangoLanguage *lang)
{
  if (pango_language_matches (lang, "zh-tw"))
    return PANGO_WIN32_COVERAGE_ZH_TW;
  else if (pango_language_matches (lang, "zh-cn"))
    return PANGO_WIN32_COVERAGE_ZH_CN;
  else if (pango_language_matches (lang, "ja"))
    return PANGO_WIN32_COVERAGE_JA;
  else if (pango_language_matches (lang, "ko"))
    return PANGO_WIN32_COVERAGE_KO;
  else if (pango_language_matches (lang, "vi"))
    return PANGO_WIN32_COVERAGE_VI;
  else
    return PANGO_WIN32_COVERAGE_UNSPEC;
}

void
pango_win32_font_entry_set_coverage (PangoWin32Face *face,
				     PangoCoverage  *coverage,
				     PangoLanguage  *lang)
{
  face->coverages[pango_win32_coverage_language_classify (lang)] = pango_coverage_ref (coverage);
}

static PangoFontDescription *
pango_win32_face_describe (PangoFontFace *face)
{
  PangoWin32Face *win32face = PANGO_WIN32_FACE (face);

  return pango_font_description_copy (win32face->description);
}

static const char *
pango_win32_face_get_face_name (PangoFontFace *face)
{
  PangoWin32Face *win32face = PANGO_WIN32_FACE (face);

  if (!win32face->face_name)
    {
      PangoFontDescription *desc = pango_font_face_describe (face);
      
      pango_font_description_unset_fields (desc,
					   PANGO_FONT_MASK_FAMILY | PANGO_FONT_MASK_SIZE);

      win32face->face_name = pango_font_description_to_string (desc);
      pango_font_description_free (desc);
    }

  return win32face->face_name;
}

static void
pango_win32_face_class_init (PangoFontFaceClass *class)
{
  class->describe = pango_win32_face_describe;
  class->get_face_name = pango_win32_face_get_face_name;
}

GType
pango_win32_face_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (PangoFontFaceClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) pango_win32_face_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (PangoWin32Face),
        0,              /* n_preallocs */
        (GInstanceInitFunc) NULL,
      };
      
      object_type = g_type_register_static (PANGO_TYPE_FONT_FACE,
                                            "PangoWin32Face",
                                            &object_info, 0);
    }
  
  return object_type;
}

PangoCoverage *
pango_win32_font_entry_get_coverage (PangoWin32Face *face,
				     PangoLanguage  *lang)
{
  gint i = pango_win32_coverage_language_classify (lang);
  if (face->coverages[i])
    {
      pango_coverage_ref (face->coverages[i]);
      return face->coverages[i];
    }

  return NULL;
}

void
pango_win32_font_entry_remove (PangoWin32Face *face,
			       PangoFont      *font)
{
  face->cached_fonts = g_slist_remove (face->cached_fonts, font);
}

/**
 * pango_win32_font_map_get_font_cache:
 * @font_map: a #PangoWin32FontMap.
 *
 * Obtains the font cache associated with the given font map.
 *
 * Returns: the #PangoWin32FontCache of @font_map.
 **/
PangoWin32FontCache *
pango_win32_font_map_get_font_cache (PangoFontMap *font_map)
{
  g_return_val_if_fail (font_map != NULL, NULL);
  g_return_val_if_fail (PANGO_WIN32_IS_FONT_MAP (font_map), NULL);

  return PANGO_WIN32_FONT_MAP (font_map)->font_cache;
}

void
pango_win32_fontmap_cache_add (PangoFontMap   *fontmap,
			       PangoWin32Font *win32font)
{
  PangoWin32FontMap *win32fontmap = PANGO_WIN32_FONT_MAP (fontmap);

  if (win32fontmap->freed_fonts->length == MAX_FREED_FONTS)
    {
      PangoWin32Font *old_font = g_queue_pop_tail (win32fontmap->freed_fonts);
      g_object_unref (old_font);
    }

  g_object_ref (win32font);
  g_queue_push_head (win32fontmap->freed_fonts, win32font);
  win32font->in_cache = TRUE;
}

void
pango_win32_fontmap_cache_remove (PangoFontMap   *fontmap,
				  PangoWin32Font *win32font)
{
  PangoWin32FontMap *win32fontmap = PANGO_WIN32_FONT_MAP (fontmap);

  GList *link = g_list_find (win32fontmap->freed_fonts->head, win32font);
  if (link == win32fontmap->freed_fonts->tail)
    {
      win32fontmap->freed_fonts->tail = win32fontmap->freed_fonts->tail->prev;
      if (win32fontmap->freed_fonts->tail)
	win32fontmap->freed_fonts->tail->next = NULL;
    }
  
  win32fontmap->freed_fonts->head = g_list_delete_link (win32fontmap->freed_fonts->head, link);
  win32fontmap->freed_fonts->length--;
  win32font->in_cache = FALSE;

  g_object_unref (win32font);
}

static void
pango_win32_fontmap_cache_clear (PangoWin32FontMap *win32fontmap)
{
  g_list_foreach (win32fontmap->freed_fonts->head, (GFunc)g_object_unref, NULL);
  g_list_free (win32fontmap->freed_fonts->head);
  win32fontmap->freed_fonts->head = NULL;
  win32fontmap->freed_fonts->tail = NULL;
  win32fontmap->freed_fonts->length = 0;
}
