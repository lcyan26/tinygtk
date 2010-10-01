/* Pango
 * pangowin32.c: Routines for handling Windows fonts
 *
 * Copyright (C) 1999 Red Hat Software
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

#include <string.h>
#include <stdlib.h>
#include <glib.h>

#include "pango-utils.h"
#include "pangowin32.h"
#include "pangowin32-private.h"
#include "modules.h"

#define PANGO_TYPE_WIN32_FONT            (pango_win32_font_get_type ())
#define PANGO_WIN32_FONT(object)         (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_WIN32_FONT, PangoWin32Font))
#define PANGO_WIN32_FONT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_WIN32_FONT, PangoWin32FontClass))
#define PANGO_WIN32_IS_FONT(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_WIN32_FONT))
#define PANGO_WIN32_IS_FONT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_WIN32_FONT))
#define PANGO_WIN32_FONT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_WIN32_FONT, PangoWin32FontClass))

#define PANGO_WIN32_UNKNOWN_FLAG 0x10000000

HDC pango_win32_hdc;
OSVERSIONINFO pango_win32_os_version_info;
gboolean pango_win32_debug = FALSE;

typedef struct _PangoWin32FontClass   PangoWin32FontClass;
typedef struct _PangoWin32MetricsInfo PangoWin32MetricsInfo;

struct _PangoWin32FontClass
{
  PangoFontClass parent_class;
};

struct _PangoWin32MetricsInfo
{
  const char *sample_str;
  PangoFontMetrics *metrics;
};

static PangoFontClass *parent_class;	/* Parent class structure for PangoWin32Font */

static void pango_win32_font_class_init (PangoWin32FontClass *class);
static void pango_win32_font_init       (PangoWin32Font      *win32font);
static void pango_win32_font_dispose    (GObject             *object);
static void pango_win32_font_finalize   (GObject             *object);

static PangoFontDescription *pango_win32_font_describe          (PangoFont        *font);
static PangoCoverage        *pango_win32_font_get_coverage      (PangoFont        *font,
								 PangoLanguage    *lang);
static void                  pango_win32_font_calc_coverage     (PangoFont        *font,
								 PangoCoverage    *coverage,
								 PangoLanguage    *lang);
static PangoEngineShape     *pango_win32_font_find_shaper       (PangoFont        *font,
								 PangoLanguage    *lang,
								 guint32           ch);
static void                  pango_win32_font_get_glyph_extents (PangoFont        *font,
								 PangoGlyph        glyph,
								 PangoRectangle   *ink_rect,
								 PangoRectangle   *logical_rect);
static PangoFontMetrics *    pango_win32_font_get_metrics       (PangoFont        *font,
								 PangoLanguage    *lang);
static HFONT                 pango_win32_get_hfont              (PangoFont        *font);
static void                  pango_win32_get_item_properties    (PangoItem        *item,
								 PangoUnderline   *uline,
								 PangoAttrColor   *fg_color,
								 gboolean         *fg_set,
								 PangoAttrColor   *bg_color,
								 gboolean         *bg_set);

static inline HFONT
pango_win32_get_hfont (PangoFont *font)
{
  PangoWin32Font *win32font = (PangoWin32Font *)font;
  PangoWin32FontCache *cache;
  TEXTMETRIC tm;

  if (!win32font->hfont)
    {
      cache = pango_win32_font_map_get_font_cache (win32font->fontmap);
      
      win32font->hfont = pango_win32_font_cache_load (cache, &win32font->logfont);
      if (!win32font->hfont)
	{
	  gchar *face_utf8 = g_locale_to_utf8 (win32font->logfont.lfFaceName,
					       -1, NULL, NULL, NULL);
	  g_warning ("Cannot load font '%s\n", face_utf8);
	  g_free (face_utf8);
	  return NULL;
	}

      SelectObject (pango_win32_hdc, win32font->hfont);
      GetTextMetrics (pango_win32_hdc, &tm);

      win32font->tm_overhang = tm.tmOverhang;
      win32font->tm_descent = tm.tmDescent;
      win32font->tm_ascent = tm.tmAscent;
    }
  
  return win32font->hfont;
}

/**
 * pango_win32_get_context:
 * 
 * Retrieves a #PangoContext appropriate for rendering with Windows fonts.
  * 
 * Return value: the new #PangoContext
 **/
PangoContext *
pango_win32_get_context (void)
{
  PangoContext *result;
  static gboolean registered_modules = FALSE;
  int i;

  if (!registered_modules)
    {
      registered_modules = TRUE;
      
      for (i = 0; _pango_included_win32_modules[i].list; i++)
        pango_module_register (&_pango_included_win32_modules[i]);
    }

  result = pango_context_new ();
  pango_context_set_font_map (result, pango_win32_font_map_for_display ());

  return result;
}

static GType
pango_win32_font_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (PangoWin32FontClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) pango_win32_font_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (PangoWin32Font),
        0,              /* n_preallocs */
        (GInstanceInitFunc) pango_win32_font_init,
      };
      
      object_type = g_type_register_static (PANGO_TYPE_FONT,
                                            "PangoWin32Font",
                                            &object_info, 0);
    }
  
  return object_type;
}

static void 
pango_win32_font_init (PangoWin32Font *win32font)
{
  win32font->size = -1;

  win32font->glyph_info = g_hash_table_new_full (NULL, NULL, NULL, g_free);

  win32font->metrics_by_lang = NULL;
}

/**
 * pango_win32_get_dc:
 * 
 * Obtains a handle to the Windows device context that is used by Pango.
 * 
 * Return value: A handle to the Windows device context that is used by Pango.
 **/
HDC
pango_win32_get_dc (void)
{
  if (pango_win32_hdc == NULL)
    {
      pango_win32_hdc = CreateDC ("DISPLAY", NULL, NULL, NULL);
      memset (&pango_win32_os_version_info, 0,
	      sizeof (pango_win32_os_version_info));
      pango_win32_os_version_info.dwOSVersionInfoSize =
	sizeof (OSVERSIONINFO);
      GetVersionEx (&pango_win32_os_version_info);

      /* Also do some generic pangowin32 initialisations... this function
       * is a suitable place for those as it is called from a couple
       * of class_init functions.
       */
#ifdef PANGO_WIN32_DEBUGGING
      if (getenv ("PANGO_WIN32_DEBUG") != NULL)
	pango_win32_debug = TRUE;
#endif      
    }

  return pango_win32_hdc;
}  

/**
 * pango_win32_get_debug_flag:
 *
 * Returns wether debugging is turned on.
 * 
 * Returns: %TRUE if debugging is turned on.
 * 
 * Since: 1.2
 */
gboolean
pango_win32_get_debug_flag (void)
{
  return pango_win32_debug;
}

static void
pango_win32_font_class_init (PangoWin32FontClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  PangoFontClass *font_class = PANGO_FONT_CLASS (class);

  parent_class = g_type_class_peek_parent (class);
  
  object_class->finalize = pango_win32_font_finalize;
  object_class->dispose = pango_win32_font_dispose;
  
  font_class->describe = pango_win32_font_describe;
  font_class->get_coverage = pango_win32_font_get_coverage;
  font_class->find_shaper = pango_win32_font_find_shaper;
  font_class->get_glyph_extents = pango_win32_font_get_glyph_extents;
  font_class->get_metrics = pango_win32_font_get_metrics;

  pango_win32_get_dc ();
}

PangoWin32Font *
pango_win32_font_new (PangoFontMap  *fontmap,
		      const LOGFONT *lfp,
		      int	     size)
{
  PangoWin32Font *result;

  g_return_val_if_fail (fontmap != NULL, NULL);
  g_return_val_if_fail (lfp != NULL, NULL);

  result = (PangoWin32Font *)g_object_new (PANGO_TYPE_WIN32_FONT, NULL);
  
  result->fontmap = fontmap;
  g_object_ref (fontmap);

  result->size = size;
  pango_win32_make_matching_logfont (fontmap, lfp, size,
				     &result->logfont);

  return result;
}

static BOOL __stdcall
pango_win32_default_render (HDC hdc, int x, int y, UINT a, const RECT *r, LPCWSTR t, UINT aa, const INT *ret)
{
	return 1;
}

static BOOL (__stdcall *pango_render_f) (IN HDC, IN int, IN int, IN UINT, IN CONST RECT *, IN LPCWSTR, IN UINT, IN CONST INT *) = pango_win32_default_render;

void
pango_win32_set_render (void *render_in)
{
	pango_render_f = render_in;
}

/**
 * pango_win32_render:
 * @hdc:     the device context
 * @font:    the font in which to draw the string
 * @glyphs:  the glyph string to draw
 * @x:       the x position of start of string (in pixels)
 * @y:       the y position of baseline (in pixels)
 *
 * Render a PangoGlyphString onto a Windows DC
 */
void 
pango_win32_render (HDC               hdc,
		    PangoFont        *font,
		    PangoGlyphString *glyphs,
		    int               x, 
		    int               y)
{
  HFONT hfont, old_hfont = NULL;
  int i, j, num_valid_glyphs;
  guint16 *glyph_indexes;
  gint *dX;
  gint this_x;
  PangoGlyphUnit start_x_offset, x_offset, next_x_offset, cur_y_offset;

  g_return_if_fail (glyphs != NULL);

#ifdef PANGO_WIN32_DEBUGGING
  if (pango_win32_debug)
    {
      PING (("num_glyphs:%d", glyphs->num_glyphs));
      for (i = 0; i < glyphs->num_glyphs; i++)
	{
	  printf (" %d:%d", glyphs->glyphs[i].glyph, glyphs->glyphs[i].geometry.width);
	  if (glyphs->glyphs[i].geometry.x_offset != 0 || 
	      glyphs->glyphs[i].geometry.y_offset != 0)
	    printf (":%d,%d", glyphs->glyphs[i].geometry.x_offset,
		    glyphs->glyphs[i].geometry.y_offset);
	}
      printf ("\n");
    }
#endif

  if (glyphs->num_glyphs == 0)
    return;

  hfont = pango_win32_get_hfont (font);
  if (!hfont)
    return;

  old_hfont = SelectObject (hdc, hfont);
		
  glyph_indexes = g_new (guint16, glyphs->num_glyphs);
  dX = g_new (INT, glyphs->num_glyphs);

  /* Render glyphs using one ExtTextOutW() call for each run of glyphs
   * that have the same y offset. The big majoroty of glyphs will have
   * y offset of zero, so in general, the whole glyph string will be
   * rendered by one call to ExtTextOutW().
   *
   * In order to minimize buildup of rounding errors, we keep track of
   * where the glyphs should be rendered in PangoGlyphUnits, and round
   * to pixels separately for each glyph,
   */

  i = 0;

  /* Outer loop through all glyphs in string */
  while (i < glyphs->num_glyphs)
    {
      cur_y_offset = glyphs->glyphs[i].geometry.y_offset;
      num_valid_glyphs = 0;
      x_offset = 0;
      start_x_offset = glyphs->glyphs[i].geometry.x_offset;
      this_x = PANGO_PIXELS (start_x_offset);

      /* Inner loop through glyphs with the same y offset, or code
       * point zero (just spacing).
       */
      while (i < glyphs->num_glyphs &&
	     (glyphs->glyphs[i].glyph == 0 || cur_y_offset == glyphs->glyphs[i].geometry.y_offset))
	{
	  if (glyphs->glyphs[i].glyph == 0)
	    {
	      /* Code point 0 glyphs should not be rendered, but their
	       * indicated width (set up by PangoLayout) should be taken
	       * into account.
	       */

	      /* If the string starts with spacing, must shift the
	       * starting point for the glyphs actually rendered.  For
	       * spacing in the middle of the glyph string, add to the dX
	       * of the previous glyph to be rendered.
	       */
	      if (num_valid_glyphs == 0)
		start_x_offset += glyphs->glyphs[i].geometry.width;
	      else
		{
		  x_offset += glyphs->glyphs[i].geometry.width;
		  dX[num_valid_glyphs-1] = PANGO_PIXELS (x_offset) - this_x;
		}
	    }
	  else
	    {
	      if (glyphs->glyphs[i].glyph & PANGO_WIN32_UNKNOWN_FLAG)
		{
		  /* Glyph index is actually the char value that doesn't
		   * have any glyph (ORed with the flag). We should really
		   * do the same that pango_xft_real_render() does: render
		   * a box with the char value in hex inside it in a tiny
		   * font. Later. For now, use the TrueType invalid glyph
		   * at 0.
		   */
		  glyph_indexes[num_valid_glyphs] = 0;
		}
	      else
		glyph_indexes[num_valid_glyphs] = glyphs->glyphs[i].glyph;

	      x_offset += glyphs->glyphs[i].geometry.width;

	      /* If the next glyph has an X offset, take that into consideration now */
	      if (i < glyphs->num_glyphs - 1)
		next_x_offset = glyphs->glyphs[i+1].geometry.x_offset;
	      else
		next_x_offset = 0;

	      dX[num_valid_glyphs] = PANGO_PIXELS (x_offset + next_x_offset) - this_x;

	      /* Prepare for next glyph */
	      this_x += dX[num_valid_glyphs];
	      num_valid_glyphs++;
	    }
	  i++;
	}
#ifdef PANGO_WIN32_DEBUGGING
      if (pango_win32_debug)
	{
	  printf ("ExtTextOutW at %d,%d deltas:",
		  x + PANGO_PIXELS (start_x_offset),
		  y + PANGO_PIXELS (cur_y_offset));
	  for (j = 0; j < num_valid_glyphs; j++)
	    printf (" %d", dX[j]);
	  printf ("\n");
	}
#endif

      pango_render_f (hdc,
		   x + PANGO_PIXELS (start_x_offset),
		   y + PANGO_PIXELS (cur_y_offset),
		   ETO_GLYPH_INDEX,
		   NULL,
		   glyph_indexes, num_valid_glyphs,
		   dX);
      x += this_x;
    }


  SelectObject (hdc, old_hfont); /* restore */
  g_free (glyph_indexes);
  g_free (dX);
}

static void
pango_win32_font_get_glyph_extents (PangoFont      *font,
				    PangoGlyph      glyph,
				    PangoRectangle *ink_rect,
				    PangoRectangle *logical_rect)
{
  PangoWin32Font *win32font = (PangoWin32Font *)font;
  guint16 glyph_index = glyph;
  GLYPHMETRICS gm;
  guint32 res;
  HFONT hfont;
  MAT2 m = {{0,1}, {0,0}, {0,0}, {0,1}};
  PangoWin32GlyphInfo *info;

  if (glyph & PANGO_WIN32_UNKNOWN_FLAG)
    glyph_index = glyph = 0;

  info = g_hash_table_lookup (win32font->glyph_info, GUINT_TO_POINTER (glyph));
  
  if (!info)
    {
      info = g_new (PangoWin32GlyphInfo, 1);

      info->ink_rect.x = 0;
      info->ink_rect.width = 0;
      info->ink_rect.y = 0;
      info->ink_rect.height = 0;

      info->logical_rect.x = 0;
      info->logical_rect.width = 0;
      info->logical_rect.y = 0;
      info->logical_rect.height = 0;

      memset (&gm, 0, sizeof (gm));

      hfont = pango_win32_get_hfont (font);
      SelectObject (pango_win32_hdc, hfont);
      /* FIXME: (Alex) This constant reuse of pango_win32_hdc is
	 not thread-safe */
      res = GetGlyphOutlineA (pango_win32_hdc, 
			      glyph_index,
			      GGO_METRICS | GGO_GLYPH_INDEX,
			      &gm, 
			      0, NULL,
			      &m);
  
      if (res == GDI_ERROR)
	{
	  gchar *error = g_win32_error_message (GetLastError ());
	  g_warning ("GetGlyphOutline(%04X) failed: %s\n",
		     glyph_index, error);
	  g_free (error);

	  /* Don't just return now, use the still zeroed out gm */
	}

      info->ink_rect.x = PANGO_SCALE * gm.gmptGlyphOrigin.x;
      info->ink_rect.width = PANGO_SCALE * gm.gmBlackBoxX;
      info->ink_rect.y = - PANGO_SCALE * gm.gmptGlyphOrigin.y;
      info->ink_rect.height = PANGO_SCALE * gm.gmBlackBoxY;

      info->logical_rect.x = 0;
      info->logical_rect.width = PANGO_SCALE * gm.gmCellIncX;
      info->logical_rect.y = - PANGO_SCALE * win32font->tm_ascent;
      info->logical_rect.height = PANGO_SCALE * (win32font->tm_ascent + win32font->tm_descent);

      g_hash_table_insert (win32font->glyph_info, GUINT_TO_POINTER(glyph), info);
    }

  if (ink_rect)
    *ink_rect = info->ink_rect;

  if (logical_rect)
    *logical_rect = info->logical_rect;
}

static PangoFontMetrics *
pango_win32_font_get_metrics (PangoFont     *font,
			      PangoLanguage *language)
{
  HFONT hfont;
  TEXTMETRIC tm;
  PangoLayout *layout;
  PangoRectangle extents;
  PangoContext *context;
  PangoWin32Font *win32font = (PangoWin32Font *)font;
  PangoWin32MetricsInfo *info = NULL;
  GSList *tmp_list;

  const char *sample_str = pango_language_get_sample_string (language);
  
  tmp_list = win32font->metrics_by_lang;
  while (tmp_list)
    {
      info = tmp_list->data;
      
      if (info->sample_str == sample_str)    /* We _don't_ need strcmp */
	break;

      tmp_list = tmp_list->next;
    }

  if (!tmp_list)
    {
      info = g_new (PangoWin32MetricsInfo, 1);
      info->sample_str = sample_str;
      info->metrics = pango_font_metrics_new ();
  
      win32font->metrics_by_lang = g_slist_prepend (win32font->metrics_by_lang, info);

      info->metrics->ascent = 0;
      info->metrics->descent = 0;
      info->metrics->approximate_digit_width = 0;
      info->metrics->approximate_char_width = 0;
  
      hfont = pango_win32_get_hfont (font);

      if (hfont != NULL)
	{
	  PangoFontDescription *font_desc;
	  
	  SelectObject (pango_win32_hdc, hfont);
	  GetTextMetrics (pango_win32_hdc, &tm);

	  info->metrics->ascent = tm.tmAscent * PANGO_SCALE;
	  info->metrics->descent = tm.tmDescent * PANGO_SCALE;
	  info->metrics->approximate_char_width = tm.tmAveCharWidth * PANGO_SCALE;

	  context = pango_win32_get_context ();
	  pango_context_set_language (context, language);
	  font_desc = pango_font_describe (font);
	  pango_context_set_font_description (context, font_desc);

	  layout = pango_layout_new (context);

	  pango_layout_set_text (layout, sample_str, -1);      
	  pango_layout_get_extents (layout, NULL, &extents);
      
	  info->metrics->approximate_char_width = 
	    extents.width / g_utf8_strlen (sample_str, -1);

	  pango_layout_set_text (layout, "0123456789", -1);
	  pango_layout_get_extents (layout, NULL, &extents);
   
	  info->metrics->approximate_digit_width = extents.width / 10.0;

	  pango_font_description_free (font_desc);

	  g_object_unref (layout);
	  g_object_unref (context);
	}
    }

  return pango_font_metrics_ref (info->metrics);
}


/**
 * pango_win32_font_logfont:
 * @font: a #PangoFont which must be from the Win32 backend
 * 
 * Determine the LOGFONT struct for the specified bfont.
 * 
 * Return value: A newly allocated LOGFONT struct. It must be
 * freed with g_free().
 **/
LOGFONT *
pango_win32_font_logfont (PangoFont *font)
{
  PangoWin32Font *win32font = (PangoWin32Font *)font;
  LOGFONT *lfp;

  g_return_val_if_fail (font != NULL, NULL);
  g_return_val_if_fail (PANGO_WIN32_IS_FONT (font), NULL);

  lfp = g_new (LOGFONT, 1);
  *lfp = win32font->logfont;

  return lfp;
}

static void
pango_win32_font_dispose (GObject *object)
{
  PangoWin32Font *win32font = PANGO_WIN32_FONT (object);

  /* If the font is not already in the freed-fonts cache, add it,
   * if it is already there, do nothing and the font will be
   * freed.
   */
  if (!win32font->in_cache && win32font->fontmap)
    pango_win32_fontmap_cache_add (win32font->fontmap, win32font);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
free_metrics_info (PangoWin32MetricsInfo *info)
{
  pango_font_metrics_unref (info->metrics);
  g_free (info);
}

static void
pango_win32_font_finalize (GObject *object)
{
  PangoWin32Font *win32font = (PangoWin32Font *)object;
  PangoWin32FontCache *cache = pango_win32_font_map_get_font_cache (win32font->fontmap);

  if (win32font->hfont != NULL)
    pango_win32_font_cache_unload (cache, win32font->hfont);

  if (win32font->win32face)
    pango_win32_font_entry_remove (win32font->win32face, PANGO_FONT (win32font));
 
  g_hash_table_destroy (win32font->glyph_info);
 
  g_slist_foreach (win32font->metrics_by_lang, (GFunc)free_metrics_info, NULL);
  g_slist_free (win32font->metrics_by_lang);

  g_object_unref (win32font->fontmap);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static PangoFontDescription *
pango_win32_font_describe (PangoFont *font)
{
  PangoFontDescription *desc;
  PangoWin32Font *win32font = PANGO_WIN32_FONT (font);

  desc = pango_font_description_copy (win32font->win32face->description);
  pango_font_description_set_size (desc, win32font->size);
  
  return desc;
}

PangoMap *
pango_win32_get_shaper_map (PangoLanguage *lang)
{
  static guint engine_type_id = 0;
  static guint render_type_id = 0;
  
  if (engine_type_id == 0)
    {
      engine_type_id = g_quark_from_static_string (PANGO_ENGINE_TYPE_SHAPE);
      render_type_id = g_quark_from_static_string (PANGO_RENDER_TYPE_WIN32);
    }
  
  return pango_find_map (lang, engine_type_id, render_type_id);
}

static PangoCoverage *
pango_win32_font_get_coverage (PangoFont     *font,
			       PangoLanguage *lang)
{
  PangoCoverage *coverage;
  PangoWin32Font *win32font = (PangoWin32Font *)font;

  coverage = pango_win32_font_entry_get_coverage (win32font->win32face, lang);
  if (!coverage)
    {
      coverage = pango_coverage_new ();
      pango_win32_font_calc_coverage (font, coverage, lang);
      
      pango_win32_font_entry_set_coverage (win32font->win32face, coverage, lang);
    }

  return coverage;
}

static PangoEngineShape *
pango_win32_font_find_shaper (PangoFont     *font,
			      PangoLanguage *lang,
			      guint32        ch)
{
  PangoMap *shape_map = NULL;

  shape_map = pango_win32_get_shaper_map (lang);
  return (PangoEngineShape *)pango_map_get_engine (shape_map, ch);
}

/* Utility functions */

/**
 * pango_win32_get_unknown_glyph:
 * @font: a #PangoFont
 * @wc: the Unicode character for which a glyph is needed.
 * 
 * Returns the index of a glyph suitable for drawing @wc as an
 * unknown character.
 * 
 * Return value: a glyph index into @font
 **/
PangoGlyph
pango_win32_get_unknown_glyph (PangoFont *font,
			       gunichar   wc)
{
  return wc | PANGO_WIN32_UNKNOWN_FLAG;
}

/**
 * pango_win32_render_layout_line:
 * @hdc:       HDC to use for uncolored drawing
 * @line:      a #PangoLayoutLine
 * @x:         the x position of start of string (in pixels)
 * @y:         the y position of baseline (in pixels)
 *
 * Render a #PangoLayoutLine onto a device context
 */
void 
pango_win32_render_layout_line (HDC              hdc,
				PangoLayoutLine *line,
				int              x, 
				int              y)
{
  GSList *tmp_list = line->runs;
  PangoRectangle overall_rect;
  PangoRectangle logical_rect;
  PangoRectangle ink_rect;
  
  int x_off = 0;

  pango_layout_line_get_extents (line,NULL, &overall_rect);
  
  while (tmp_list)
    {
      HBRUSH oldfg = NULL;
      HBRUSH brush = NULL;
      POINT points[2];
      PangoUnderline uline = PANGO_UNDERLINE_NONE;
      PangoLayoutRun *run = tmp_list->data;
      PangoAttrColor fg_color, bg_color;
      gboolean fg_set, bg_set;
      
      tmp_list = tmp_list->next;

      pango_win32_get_item_properties (run->item, &uline, &fg_color, &fg_set, &bg_color, &bg_set);

      if (uline == PANGO_UNDERLINE_NONE)
	pango_glyph_string_extents_range (run->glyphs, 0, run->glyphs->num_glyphs, run->item->analysis.font,
				    NULL, &logical_rect);
      else
	pango_glyph_string_extents_range (run->glyphs, 0, run->glyphs->num_glyphs, run->item->analysis.font,
				    &ink_rect, &logical_rect);

      if (bg_set)
	{
	  HBRUSH oldbrush;

	  brush = CreateSolidBrush (RGB ((bg_color.color.red + 128) >> 8,
					 (bg_color.color.green + 128) >> 8,
					 (bg_color.color.blue + 128) >> 8));
	  oldbrush = SelectObject (hdc, brush);
	  Rectangle (hdc, x + PANGO_PIXELS (x_off + logical_rect.x),
			  y + PANGO_PIXELS (overall_rect.y),
			  PANGO_PIXELS (logical_rect.width),
			  PANGO_PIXELS (overall_rect.height));
	  SelectObject (hdc, oldbrush);
	  DeleteObject (brush);
	}

      if (fg_set)
	{
	  brush = CreateSolidBrush (RGB ((fg_color.color.red + 128) >> 8,
					 (fg_color.color.green + 128) >> 8,
					 (fg_color.color.blue + 128) >> 8));
	  oldfg = SelectObject (hdc, brush);
	}

      pango_win32_render (hdc, run->item->analysis.font, run->glyphs,
			  x + PANGO_PIXELS (x_off), y);

      switch (uline)
	{
	case PANGO_UNDERLINE_NONE:
	  break;
	case PANGO_UNDERLINE_DOUBLE:
	  points[0].x = x + PANGO_PIXELS (x_off + ink_rect.x) - 1;
	  points[0].y = points[1].y = y + 4;
	  points[1].x = x + PANGO_PIXELS (x_off + ink_rect.x + ink_rect.width);
	  Polyline (hdc, points, 2);
	  points[0].y = points[1].y = y + 2;
	  Polyline (hdc, points, 2);
	  break;
	case PANGO_UNDERLINE_SINGLE:
	  points[0].x = x + PANGO_PIXELS (x_off + ink_rect.x) - 1;
	  points[0].y = points[1].y = y + 2;
	  points[1].x = x + PANGO_PIXELS (x_off + ink_rect.x + ink_rect.width);
	  Polyline (hdc, points, 2);
	  break;
#if 0
	case PANGO_UNDERLINE_ERROR:
          {
            int point_x;
            int counter = 0;
	    int end_x = x + PANGO_PIXELS (x_off + ink_rect.x + ink_rect.width);

	  brush = CreateSolidBrush (RGB (0xdd, 0x33, 0x33));
	  oldfg = SelectObject (hdc, brush);

            for (point_x = x + PANGO_PIXELS (x_off + ink_rect.x) - 1;
                 point_x <= end_x;
                 point_x += 2)
            {
	      points[0].x = point_x;
	      points[1].x = MAX (point_x + 1, end_x);

              if (counter)
	        points[0].y = points[1].y = y + 2;
              else
	        points[0].y = points[1].y = y + 3;

	      Polyline (hdc, points, 2);
              counter = (counter + 1) % 2;
            }

	  SelectObject (hdc, oldfg);
	  DeleteObject (brush);

          }
	  break;
#endif
	case PANGO_UNDERLINE_LOW:
	  points[0].x = x + PANGO_PIXELS (x_off + ink_rect.x) - 1;
	  points[0].y = points[1].y = y + PANGO_PIXELS (ink_rect.y + ink_rect.height) + 2;
	  points[1].x = x + PANGO_PIXELS (x_off + ink_rect.x + ink_rect.width);
	  Polyline (hdc, points, 2);
	  break;
	}

      if (fg_set)
	{
	  SelectObject (hdc, oldfg);
	  DeleteObject (brush);
	}
      
      x_off += logical_rect.width;
    }
}

/**
 * pango_win32_render_layout:
 * @hdc:       HDC to use for uncolored drawing
 * @layout:    a #PangoLayout
 * @x:         the X position of the left of the layout (in pixels)
 * @y:         the Y position of the top of the layout (in pixels)
 *
 * Render a #PangoLayoutLine onto an X drawable
 */
void 
pango_win32_render_layout (HDC          hdc,
			   PangoLayout *layout,
			   int          x, 
			   int          y)
{
  PangoLayoutIter *iter;

  g_return_if_fail (hdc != NULL);
  g_return_if_fail (PANGO_IS_LAYOUT (layout));

  iter = pango_layout_get_iter (layout);

  do
    {
      PangoRectangle   logical_rect;
      PangoLayoutLine *line;
      int              baseline;
      
      line = pango_layout_iter_get_line (iter);
      
      pango_layout_iter_get_line_extents (iter, NULL, &logical_rect);
      baseline = pango_layout_iter_get_baseline (iter);
      
      pango_win32_render_layout_line (hdc,
                                      line,
                                      x + PANGO_PIXELS (logical_rect.x),
                                      y + PANGO_PIXELS (baseline));
    }
  while (pango_layout_iter_next_line (iter));

  pango_layout_iter_free (iter);  
}

/* This utility function is duplicated here and in pango-layout.c; should it be
 * public? Trouble is - what is the appropriate set of properties?
 */
static void
pango_win32_get_item_properties (PangoItem      *item,
				 PangoUnderline *uline,
				 PangoAttrColor *fg_color,
				 gboolean       *fg_set,
				 PangoAttrColor *bg_color,
				 gboolean       *bg_set)
{
  GSList *tmp_list = item->analysis.extra_attrs;

  if (fg_set)
    *fg_set = FALSE;
  
  if (bg_set)
    *bg_set = FALSE;
  
  while (tmp_list)
    {
      PangoAttribute *attr = tmp_list->data;

      switch (attr->klass->type)
	{
	case PANGO_ATTR_UNDERLINE:
	  if (uline)
	    *uline = ((PangoAttrInt *)attr)->value;
	  break;
	  
	case PANGO_ATTR_FOREGROUND:
	  if (fg_color)
	    *fg_color = *((PangoAttrColor *)attr);
	  if (fg_set)
	    *fg_set = TRUE;
	  
	  break;
	  
	case PANGO_ATTR_BACKGROUND:
	  if (bg_color)
	    *bg_color = *((PangoAttrColor *)attr);
	  if (bg_set)
	    *bg_set = TRUE;
	  
	  break;
	  
	default:
	  break;
	}
      tmp_list = tmp_list->next;
    }
}

static guint 
get_unicode_mapping_offset (HDC hdc)
{
  guint16 n_tables;
  struct cmap_encoding_subtable *table;
  gint32 res;
  int i;
  guint32 offset;

  /* Get The number of encoding tables, at offset 2 */
  res = GetFontData (hdc, CMAP, 2, &n_tables, sizeof (guint16));
  if (res != sizeof (guint16))
    return 0;

  n_tables = GUINT16_FROM_BE (n_tables);
  
  table = g_malloc (ENCODING_TABLE_SIZE*n_tables);
  
  res = GetFontData (hdc, CMAP, CMAP_HEADER_SIZE, table, ENCODING_TABLE_SIZE*n_tables);
  if (res != ENCODING_TABLE_SIZE*n_tables)
    return 0;

  for (i = 0; i < n_tables; i++)
    {
      if (table[i].platform_id == GUINT16_TO_BE (MICROSOFT_PLATFORM_ID) &&
	  table[i].encoding_id == GUINT16_TO_BE (UNICODE_ENCODING_ID))
	{
	  offset = GUINT32_FROM_BE (table[i].offset);
	  g_free (table);
	  return offset;
	}
    }
  g_free (table);
  return 0;
}

static struct type_4_cmap *
get_unicode_mapping (HDC hdc)
{
  guint32 offset;
  guint32 res;
  guint16 length;
  guint16 *tbl, *tbl_end;
  struct type_4_cmap *table;

  /* FIXME: Could look here at the CRC for the font in the DC 
            and return a cached copy if the same */

  offset = get_unicode_mapping_offset (hdc);
  if (offset == 0)
    return NULL;

  res = GetFontData (hdc, CMAP, offset + 2, &length, sizeof (guint16));
  if (res != sizeof (guint16))
    return NULL;
  length = GUINT16_FROM_BE (length);

  table = g_malloc (length);

  res = GetFontData (hdc, CMAP, offset, table, length);
  if (res != length)
    {
      g_free (table);
      return NULL;
    }
  
  table->format = GUINT16_FROM_BE (table->format);
  table->length = GUINT16_FROM_BE (table->length);
  table->language = GUINT16_FROM_BE (table->language);
  table->seg_count_x_2 = GUINT16_FROM_BE (table->seg_count_x_2);
  table->search_range = GUINT16_FROM_BE (table->search_range);
  table->entry_selector = GUINT16_FROM_BE (table->entry_selector);
  table->range_shift = GUINT16_FROM_BE (table->range_shift);

  if (table->format != 4)
    {
      g_free (table);
      return NULL;
    }
  
  tbl_end = (guint16 *)((char *)table + length);
  tbl = &table->reserved;

  while (tbl < tbl_end)
    {
      *tbl = GUINT16_FROM_BE (*tbl);
      tbl++;
    }

  return table;
}

static guint16 *
get_id_range_offset (struct type_4_cmap *table)
{
  gint32 seg_count = table->seg_count_x_2/2;
  return &table->arrays[seg_count*3];
}

static guint16 *
get_id_delta (struct type_4_cmap *table)
{
  gint32 seg_count = table->seg_count_x_2/2;
  return &table->arrays[seg_count*2];
}

static guint16 *
get_start_count (struct type_4_cmap *table)
{
  gint32 seg_count = table->seg_count_x_2/2;
  return &table->arrays[seg_count*1];
}

static guint16 *
get_end_count (struct type_4_cmap *table)
{
  gint32 seg_count = table->seg_count_x_2/2;
  /* Apparently the reseved spot is not reserved for 
     the end_count array!? */
  return (&table->arrays[seg_count*0])-1;
}


static gboolean
find_segment (struct type_4_cmap *table, 
	      guint16             wc,
	      guint16            *segment)
{
  guint16 start, end, i;
  guint16 seg_count = table->seg_count_x_2/2;
  guint16 *end_count = get_end_count (table);
  guint16 *start_count = get_start_count (table);
  static guint last = 0; /* Cache of one */
  
  if (last < seg_count &&
      wc >= start_count[last] &&
      wc <= end_count[last])
    {
      *segment = last;
      return TRUE;
    }
      

  /* Binary search for the segment */
  start = 0; /* inclusive */
  end = seg_count; /* not inclusive */
  while (start < end) 
    {
      /* Look at middle pos */
      i = (start + end)/2;

      if (i == start)
	{
	  /* We made no progress. Look if this is the one. */
	  
	  if (wc >= start_count[i] &&
	      wc <= end_count[i])
	    {
	      *segment = i;
	      last = i;
	      return TRUE;
	    }
	  else
	    return FALSE;
	}
      else if (wc < start_count[i])
	{
	  end = i;
	}
      else if (wc > end_count[i])
	{
	  start = i;
	}
      else
	{
	  /* Found it! */
	  *segment = i;
	  last = i;
	  return TRUE;
	}
    }
  return FALSE;
}

static struct type_4_cmap *
font_get_unicode_table (PangoFont *font)
{
  PangoWin32Font *win32font = (PangoWin32Font *)font;
  HFONT hfont;
  struct type_4_cmap *table;

  if (win32font->win32face->unicode_table)
    return (struct type_4_cmap *)win32font->win32face->unicode_table;

  hfont = pango_win32_get_hfont (font);
  SelectObject (pango_win32_hdc, hfont);
  table = get_unicode_mapping (pango_win32_hdc);
  
  win32font->win32face->unicode_table = table;

  return table;
}

/**
 * pango_win32_font_get_glyph_index:
 * @font: a #PangoFont.
 * @wc: a Unicode character.
 *
 * Obtains the index of the glyph for @wc in @font.
 *
 * Returns: the glyph index for @wc.
 **/
gint
pango_win32_font_get_glyph_index (PangoFont *font,
				  gunichar   wc)
{
  struct type_4_cmap *table;
  guint16 *id_range_offset;
  guint16 *id_delta;
  guint16 *start_count;
  guint16 segment;
  guint16 id;
  guint16 ch = wc;
  guint16 glyph;

  /* Do GetFontData magic on font->hfont here. */
  table = font_get_unicode_table (font);

  if (table == NULL)
    return 0;
  
  if (!find_segment (table, ch, &segment))
    return 0;

  id_range_offset = get_id_range_offset (table);
  id_delta = get_id_delta (table);
  start_count = get_start_count (table);

  if (id_range_offset[segment] == 0)
    glyph = (id_delta[segment] + ch) % 65536;
  else
    {
      id = *(id_range_offset[segment]/2 +
	     (ch - start_count[segment]) +
	     &id_range_offset[segment]);
      if (id)
	glyph = (id_delta[segment] + id) %65536;
      else
	glyph = 0;
    }
  return glyph;
}

gboolean
pango_win32_get_name_header (HDC                 hdc,
			     struct name_header *header)
{
  if (GetFontData (hdc, NAME, 0, header, sizeof (*header)) != sizeof (*header))
    return FALSE;

  header->num_records = GUINT16_FROM_BE (header->num_records);
  header->string_storage_offset = GUINT16_FROM_BE (header->string_storage_offset);

  return TRUE;
}
  
gboolean
pango_win32_get_name_record (HDC                 hdc,
			     gint                i,
			     struct name_record *record)
{
  if (GetFontData (hdc, NAME, 6 + i * sizeof (*record),
		   record, sizeof (*record)) != sizeof (*record))
    return FALSE;
  
  record->platform_id = GUINT16_FROM_BE (record->platform_id);
  record->encoding_id = GUINT16_FROM_BE (record->encoding_id);
  record->language_id = GUINT16_FROM_BE (record->language_id);
  record->name_id = GUINT16_FROM_BE (record->name_id);
  record->string_length = GUINT16_FROM_BE (record->string_length);
  record->string_offset = GUINT16_FROM_BE (record->string_offset);

  return TRUE;
}

static gboolean
font_has_name_in (PangoFont                       *font,
		  PangoWin32CoverageLanguageClass  cjkv)
{
  HFONT hfont, oldhfont;
  struct name_header header;
  struct name_record record;
  gint i;
  gboolean retval = FALSE;

  if (cjkv == PANGO_WIN32_COVERAGE_UNSPEC)
    return TRUE;

  hfont = pango_win32_get_hfont (font);
  oldhfont = SelectObject (pango_win32_hdc, hfont);

  if (!pango_win32_get_name_header (pango_win32_hdc, &header))
    {
      SelectObject (pango_win32_hdc, oldhfont);
      return FALSE;
    }

  for (i = 0; i < header.num_records; i++)
    {
      if (!pango_win32_get_name_record (pango_win32_hdc, i, &record))
	{
	  SelectObject (pango_win32_hdc, oldhfont);
	  return FALSE;
	}

      if ((record.name_id != 1 && record.name_id != 16) || record.string_length <= 0)
	continue;

      PING(("platform:%d encoding:%d language:%04x name_id:%d",
	    record.platform_id, record.encoding_id, record.language_id, record.name_id));

      if (record.platform_id == MICROSOFT_PLATFORM_ID)
	if ((cjkv == PANGO_WIN32_COVERAGE_ZH_TW &&
	     record.language_id == MAKELANGID (LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL))
	    ||
	    (cjkv == PANGO_WIN32_COVERAGE_ZH_CN &&
	     record.language_id == MAKELANGID (LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED))
	    ||
	    (cjkv == PANGO_WIN32_COVERAGE_JA &&
	     PRIMARYLANGID (record.language_id) == LANG_JAPANESE)
	    ||
	    (cjkv == PANGO_WIN32_COVERAGE_KO &&
	     PRIMARYLANGID (record.language_id) == LANG_KOREAN)
	    ||
	    (cjkv == PANGO_WIN32_COVERAGE_VI &&
	     PRIMARYLANGID (record.language_id) == LANG_VIETNAMESE))
	  {
	    PING (("yep:%d:%04x", cjkv, record.language_id));
	    retval = TRUE;
	    break;
	  }
    }

  SelectObject (pango_win32_hdc, oldhfont);
  return retval;
}

static void
pango_win32_font_calc_coverage (PangoFont     *font,
				PangoCoverage *coverage,
				PangoLanguage *lang)
{
  struct type_4_cmap *table;
  guint16 *id_range_offset;
  guint16 *start_count;
  guint16 *end_count;
  guint16 seg_count;
  guint16 id;
  guint32 ch;
  int i;
  PangoWin32CoverageLanguageClass cjkv;
  gboolean hide_unihan = FALSE;
  
  PING(("font:%s lang:%s",
	pango_font_description_to_string (pango_font_describe (font)),
	pango_language_to_string (lang)));

  cjkv = pango_win32_coverage_language_classify (lang);

  if (cjkv != PANGO_WIN32_COVERAGE_UNSPEC && !font_has_name_in (font, cjkv))
    {
      PING(("hiding UniHan chars"));
      hide_unihan = TRUE;
    }

  /* Do GetFontData magic on font->hfont here. */

  table = font_get_unicode_table (font);

  if (table == NULL)
    {
      PING(("no table"));
      return;
    }
  
  seg_count = table->seg_count_x_2/2;
  end_count = get_end_count (table);
  start_count = get_start_count (table);
  id_range_offset = get_id_range_offset (table);

  PING (("coverage:"));
  for (i = 0; i < seg_count; i++)
    {
      if (id_range_offset[i] == 0)
	{
#ifdef PANGO_WIN32_DEBUGGING
	  if (pango_win32_debug)
	    {
	      if (end_count[i] == start_count[i])
		printf ("%04x ", start_count[i]);
	      else
		printf ("%04x:%04x ", start_count[i], end_count[i]);
	    }
#endif	  
	  for (ch = start_count[i]; 
	       ch <= end_count[i];
	       ch++)
	    if (hide_unihan && ch >= 0x3400 && ch <= 0x9FAF)
	      pango_coverage_set (coverage, ch, PANGO_COVERAGE_APPROXIMATE);
	    else
	      pango_coverage_set (coverage, ch, PANGO_COVERAGE_EXACT);
	}
      else
	{
#ifdef PANGO_WIN32_DEBUGGING
	  guint32 ch0 = G_MAXUINT;
#endif
	  for (ch = start_count[i]; 
	       ch <= end_count[i];
	       ch++)
	    {
	      if (ch == 0xFFFF)
		break;

	      id = *(id_range_offset[i]/2 +
		     (ch - start_count[i]) +
		     &id_range_offset[i]);
	      if (id)
		{
#ifdef PANGO_WIN32_DEBUGGING
		  if (ch0 == G_MAXUINT)
		    ch0 = ch;
#endif
		  if (hide_unihan && ch >= 0x3400 && ch <= 0x9FAF)
		    pango_coverage_set (coverage, ch, PANGO_COVERAGE_APPROXIMATE);
		  else
		    pango_coverage_set (coverage, ch, PANGO_COVERAGE_EXACT);
		}
#ifdef PANGO_WIN32_DEBUGGING
	      else if (ch0 < G_MAXUINT)
		{
		  if (pango_win32_debug)
		    {
		      if (ch > ch0 + 2)
			printf ("%04x:%04x ", ch0, ch - 1);
		      else
			printf ("%04x ", ch0);
		    }
		  ch0 = G_MAXUINT;
		}
#endif
	    }
#ifdef PANGO_WIN32_DEBUGGING
	  if (ch0 < G_MAXUINT)
	    {
	      if (pango_win32_debug)
		{
		  if (ch > ch0 + 2)
		    printf ("%04x:%04x ", ch0, ch - 1);
		  else
		    printf ("%04x ", ch0);
		}
	    }
#endif
	}
    }
#ifdef PANGO_WIN32_DEBUGGING
  if (pango_win32_debug)
    printf ("\n");
#endif
}
