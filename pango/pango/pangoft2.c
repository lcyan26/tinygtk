/* Pango
 * pangoft2.c: Routines for handling FreeType2 fonts
 *
 * Copyright (C) 1999 Red Hat Software
 * Copyright (C) 2000 Tor Lillqvist
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
#include <math.h>
#include <glib.h>
#include <glib/gprintf.h>

#include <freetype/freetype.h>

#include "pango-utils.h"
#include "pangoft2.h"
#include "pangoft2-private.h"

/* for compatibility with older freetype versions */
#ifndef FT_LOAD_TARGET_MONO
#define FT_LOAD_TARGET_MONO  FT_LOAD_MONOCHROME
#endif

#define PANGO_TYPE_FT2_FONT              (pango_ft2_font_get_type ())
#define PANGO_FT2_FONT(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_FT2_FONT, PangoFT2Font))
#define PANGO_FT2_FONT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_FT2_FONT, PangoFT2FontClass))
#define PANGO_FT2_IS_FONT(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_FT2_FONT))
#define PANGO_FT2_IS_FONT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_FT2_FONT))
#define PANGO_FT2_FONT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_FT2_FONT, PangoFT2FontClass))

typedef struct _PangoFT2FontClass   PangoFT2FontClass;
typedef struct _PangoFT2MetricsInfo PangoFT2MetricsInfo;

struct _PangoFT2FontClass
{
  PangoFontClass parent_class;
};

struct _PangoFT2MetricsInfo
{
  const char       *sample_str;
  PangoFontMetrics *metrics;
};

typedef struct
{
  FT_Bitmap bitmap;
  int bitmap_left;
  int bitmap_top;
} PangoFT2RenderedGlyph;

static PangoFontClass *parent_class;	/* Parent class structure for PangoFT2Font */

static void pango_ft2_font_class_init (PangoFT2FontClass *class);
static void pango_ft2_font_init       (PangoFT2Font      *ft2font);
static void pango_ft2_font_finalize   (GObject         *object);

static PangoFontDescription *pango_ft2_font_describe          (PangoFont      *font);

static PangoEngineShape *    pango_ft2_font_find_shaper       (PangoFont      *font,
							       PangoLanguage  *language,
							       guint32         ch);

static void                  pango_ft2_font_get_glyph_extents (PangoFont      *font,
							       PangoGlyph      glyph,
							       PangoRectangle *ink_rect,
							       PangoRectangle *logical_rect);

static PangoFontMetrics *    pango_ft2_font_get_metrics       (PangoFont      *font,
							       PangoLanguage  *language);
  
static void                  pango_ft2_get_item_properties    (PangoItem      *item,
							       PangoUnderline *uline,
							       gboolean       *strikethrough,
							       gint           *rise,
							       gboolean       *shape_set,
							       PangoRectangle *ink_rect,
							       PangoRectangle *logical_rect);


static GType pango_ft2_font_get_type (void);

PangoFT2Font *
_pango_ft2_font_new (PangoFontMap    *fontmap,
		     FcPattern  *pattern)
{
  PangoFT2Font *ft2font;
  double d;

  g_return_val_if_fail (fontmap != NULL, NULL);
  g_return_val_if_fail (pattern != NULL, NULL);

  ft2font = (PangoFT2Font *)g_object_new (PANGO_TYPE_FT2_FONT, NULL);

  ft2font->fontmap = fontmap;
  ft2font->font_pattern = pattern;
  
  g_object_ref (fontmap);
  ft2font->description = _pango_ft2_font_desc_from_pattern (pattern, TRUE);
  ft2font->face = NULL;

  if (FcPatternGetDouble (pattern, FC_PIXEL_SIZE, 0, &d) == FcResultMatch)
    ft2font->size = d*PANGO_SCALE;

  _pango_ft2_font_map_add (ft2font->fontmap, ft2font);

  return ft2font;
}

static gboolean
set_unicode_charmap (FT_Face face)
{
  int charmap;

  for (charmap = 0; charmap < face->num_charmaps; charmap++)
    if (face->charmaps[charmap]->encoding == ft_encoding_unicode)
      {
	FT_Error error = FT_Set_Charmap(face, face->charmaps[charmap]);
	return error == FT_Err_Ok;
      }

  return FALSE;
}

static void
load_fallback_face (PangoFT2Font *ft2font,
		    const char   *original_file)
{
  FcPattern *sans;
  FcPattern *matched;
  FcResult result;
  FT_Error error;
  FcChar8 *filename2 = NULL;
  gchar *name;
  int id;
  
  sans = FcPatternBuild (NULL,
			 FC_FAMILY, FcTypeString, "sans",
			 FC_SIZE, FcTypeDouble, (double)pango_font_description_get_size (ft2font->description)/PANGO_SCALE,
			 NULL);
  
  matched = FcFontMatch (0, sans, &result);
  
  if (FcPatternGetString (matched, FC_FILE, 0, &filename2) != FcResultMatch)
    goto bail1;
  
  if (FcPatternGetInteger (matched, FC_INDEX, 0, &id) != FcResultMatch)
    goto bail1;
  
  error = FT_New_Face (_pango_ft2_font_map_get_library (ft2font->fontmap),
		       (char *) filename2, id, &ft2font->face);
  
  
  if (error)
    {
    bail1:
      name = pango_font_description_to_string (ft2font->description);
      g_warning ("Unable to open font file %s for font %s, exiting\n", filename2, name);
      exit (1);
    }
  else
    {
      name = pango_font_description_to_string (ft2font->description);
      g_warning ("Unable to open font file %s for font %s, falling back to %s\n", original_file, name, filename2);
      g_free (name);
    }

  if (!set_unicode_charmap (ft2font->face))
    {
      g_warning ("Unable to load unicode charmap from file %s, exiting\n", filename2);
      exit (1);
    }
  
  FcPatternDestroy (sans);
  FcPatternDestroy (matched);
}

/**
 * pango_ft2_font_get_face:
 * @font: a #PangoFont
 * 
 * Returns the native FreeType2 FT_Face structure used for this PangoFont.
 * This may be useful if you want to use FreeType2 functions directly.
 * 
 * Return value: a pointer to a #FT_Face structure, with the size set correctly
 **/
FT_Face
pango_ft2_font_get_face (PangoFont *font)
{
  PangoFT2Font *ft2font = (PangoFT2Font *)font;
  FT_Error error;
  FcPattern *pattern;
  FcChar8 *filename;
  FcBool antialias, hinting, autohint;
  int id;

  pattern = ft2font->font_pattern;

  if (!ft2font->face)
    {
      ft2font->load_flags = 0;

      /* disable antialiasing if requested */
      if (FcPatternGetBool (pattern,
                            FC_ANTIALIAS, 0, &antialias) != FcResultMatch)
	antialias = FcTrue;

      if (antialias)
        ft2font->load_flags |= FT_LOAD_NO_BITMAP;
      else
	ft2font->load_flags |= FT_LOAD_TARGET_MONO;

      /* disable hinting if requested */
      if (FcPatternGetBool (pattern,
                            FC_HINTING, 0, &hinting) != FcResultMatch)
	hinting = FcTrue;

      if (!hinting)
        ft2font->load_flags |= FT_LOAD_NO_HINTING;

      /* force autohinting if requested */
      if (FcPatternGetBool (pattern,
                            FC_AUTOHINT, 0, &autohint) != FcResultMatch)
	autohint = FcFalse;

      if (autohint)
        ft2font->load_flags |= FT_LOAD_FORCE_AUTOHINT;

      if (FcPatternGetString (pattern, FC_FILE, 0, &filename) != FcResultMatch)
	      goto bail0;
      
      if (FcPatternGetInteger (pattern, FC_INDEX, 0, &id) != FcResultMatch)
	      goto bail0;

      error = FT_New_Face (_pango_ft2_font_map_get_library (ft2font->fontmap),
			   (char *) filename, id, &ft2font->face);
      if (error != FT_Err_Ok)
	{
	bail0:
	  load_fallback_face (ft2font, filename);
	}

      g_assert (ft2font->face);
      
      if (!set_unicode_charmap (ft2font->face))
	{
	  g_warning ("Unable to load unicode charmap from font file %s",
                     filename);
	  
	  FT_Done_Face (ft2font->face);
	  ft2font->face = NULL;
	  
	  load_fallback_face (ft2font, filename);
	}

      error = FT_Set_Char_Size (ft2font->face,
				PANGO_PIXELS_26_6 (ft2font->size),
				PANGO_PIXELS_26_6 (ft2font->size),
				0, 0);
      if (error)
	g_warning ("Error in FT_Set_Char_Size: %d", error);
    }
  
  return ft2font->face;
}

static GType
pango_ft2_font_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (PangoFT2FontClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) pango_ft2_font_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (PangoFT2Font),
        0,              /* n_preallocs */
        (GInstanceInitFunc) pango_ft2_font_init,
      };
      
      object_type = g_type_register_static (PANGO_TYPE_FONT,
                                            "PangoFT2Font",
                                            &object_info, 0);
    }
  
  return object_type;
}

static void 
pango_ft2_font_init (PangoFT2Font *ft2font)
{
  ft2font->face = NULL;

  ft2font->size = 0;

  ft2font->metrics_by_lang = NULL;

  ft2font->glyph_info = g_hash_table_new (NULL, NULL);
}

static void
pango_ft2_font_class_init (PangoFT2FontClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  PangoFontClass *font_class = PANGO_FONT_CLASS (class);

  parent_class = g_type_class_peek_parent (class);
  
  object_class->finalize = pango_ft2_font_finalize;
  
  font_class->describe = pango_ft2_font_describe;
  font_class->get_coverage = pango_ft2_font_get_coverage;
  font_class->find_shaper = pango_ft2_font_find_shaper;
  font_class->get_glyph_extents = pango_ft2_font_get_glyph_extents;
  font_class->get_metrics = pango_ft2_font_get_metrics;
}

static void
pango_ft2_free_rendered_glyph (PangoFT2RenderedGlyph *rendered)
{
  g_free (rendered->bitmap.buffer);
  g_free (rendered);
}

static PangoFT2RenderedGlyph *
pango_ft2_font_render_glyph (PangoFont *font,
			     int glyph_index)
{
  PangoFT2RenderedGlyph *rendered;
  FT_Face face;

  rendered = g_new (PangoFT2RenderedGlyph, 1);

  face = pango_ft2_font_get_face (font);
  
  if (face)
    {
      PangoFT2Font *ft2font = (PangoFT2Font *) font;

      /* Draw glyph */
      FT_Load_Glyph (face, glyph_index, ft2font->load_flags);
      FT_Render_Glyph (face->glyph,
		       (ft2font->load_flags & FT_LOAD_TARGET_MONO ?
			ft_render_mode_mono : ft_render_mode_normal));

      rendered->bitmap = face->glyph->bitmap;
      rendered->bitmap.buffer = g_memdup (face->glyph->bitmap.buffer,
					  face->glyph->bitmap.rows * face->glyph->bitmap.pitch);
      rendered->bitmap_left = face->glyph->bitmap_left;
      rendered->bitmap_top = face->glyph->bitmap_top;
    }
  else
    g_error ("Couldn't get face for PangoFT2Face");

  return rendered;
}


/**
 * pango_ft2_render:
 * @bitmap:  the FreeType2 bitmap onto which to draw the string
 * @font:    the font in which to draw the string
 * @glyphs:  the glyph string to draw
 * @x:       the x position of the start of the string (in pixels)
 * @y:       the y position of the baseline (in pixels)
 *
 * Renders a PangoGlyphString onto a FreeType2 bitmap.
 **/
void 
pango_ft2_render (FT_Bitmap        *bitmap,
		  PangoFont        *font,
		  PangoGlyphString *glyphs,
		  int               x, 
		  int               y)
{
  FT_UInt glyph_index;
  int i;
  int x_position = 0;
  int ix, iy, ixoff, iyoff, y_start, y_limit, x_start, x_limit;
  PangoGlyphInfo *gi;
  guchar *dest, *src;
  gboolean add_glyph_to_cache;

  g_return_if_fail (bitmap != NULL);
  g_return_if_fail (glyphs != NULL);

  PING (("bitmap: %dx%d@+%d+%d", bitmap->width, bitmap->rows, x, y));

  gi = glyphs->glyphs;
  for (i = 0; i < glyphs->num_glyphs; i++, gi++)
    {
      if (gi->glyph)
	{
	  PangoFT2RenderedGlyph *rendered_glyph;
	  glyph_index = gi->glyph;

	  rendered_glyph = pango_ft2_font_get_cache_glyph_data (font,
								glyph_index);
	  add_glyph_to_cache = FALSE;
	  if (rendered_glyph == NULL)
	    {
	      rendered_glyph = pango_ft2_font_render_glyph (font, glyph_index);
	      add_glyph_to_cache = TRUE;
	    }
	  
	  ixoff = x + PANGO_PIXELS (x_position + gi->geometry.x_offset);
	  iyoff = y + PANGO_PIXELS (gi->geometry.y_offset);
	  
	  x_start = MAX (0, - (ixoff + rendered_glyph->bitmap_left));
	  x_limit = MIN (rendered_glyph->bitmap.width,
			 bitmap->width - (ixoff + rendered_glyph->bitmap_left));

	  y_start = MAX (0,  - (iyoff - rendered_glyph->bitmap_top));
	  y_limit = MIN (rendered_glyph->bitmap.rows,
			 bitmap->rows - (iyoff - rendered_glyph->bitmap_top));

	  PING (("glyph %d:%d: bitmap: %dx%d, left:%d top:%d",
		 i, glyph_index,
		 rendered_glyph->bitmap.width, rendered_glyph->bitmap.rows,
		 rendered_glyph->bitmap_left, rendered_glyph->bitmap_top));
	  PING (("xstart:%d xlim:%d ystart:%d ylim:%d",
		 x_start, x_limit, y_start, y_limit));

	  src = rendered_glyph->bitmap.buffer +
	    y_start * rendered_glyph->bitmap.pitch;

	  dest = bitmap->buffer +
	    (y_start + iyoff - rendered_glyph->bitmap_top) * bitmap->pitch +
	    x_start + ixoff + rendered_glyph->bitmap_left;

	  switch (rendered_glyph->bitmap.pixel_mode)
	    {
	    case ft_pixel_mode_grays:
	      src += x_start;
	      for (iy = y_start; iy < y_limit; iy++)
		{
		  guchar *s = src;
		  guchar *d = dest;

		  for (ix = x_start; ix < x_limit; ix++)
		    {
		      switch (*s)
			{
			case 0:
			  break;
			case 0xff:
			  *d = 0xff;
			default:
			  *d = MIN ((gushort) *d + (gushort) *s, 0xff);
			  break;
			}

		      s++;
		      d++;
		    }

		  dest += bitmap->pitch;
		  src  += rendered_glyph->bitmap.pitch;
		}
	      break;

	    case ft_pixel_mode_mono:
	      src += x_start / 8;
	      for (iy = y_start; iy < y_limit; iy++)
		{
		  guchar *s = src;
		  guchar *d = dest;

		  for (ix = x_start; ix < x_limit; ix++)
		    {
		      if ((*s) & (1 << (7 - (ix % 8))))
			*d |= 0xff;
		      
		      if ((ix % 8) == 7)
			s++;
		      d++;
		    }

		  dest += bitmap->pitch;
		  src  += rendered_glyph->bitmap.pitch;
		}
	      break;
	      
	    default:
	      g_warning ("pango_ft2_render: "
			 "Unrecognized glyph bitmap pixel mode %d\n",
			 rendered_glyph->bitmap.pixel_mode);
	      break;
	    }

	  if (add_glyph_to_cache)
	    {
	      pango_ft2_font_set_glyph_cache_destroy (font,
						      (GDestroyNotify) pango_ft2_free_rendered_glyph);
	      pango_ft2_font_set_cache_glyph_data (font,
						   glyph_index, rendered_glyph);
	    }
	}

      x_position += glyphs->glyphs[i].geometry.width;
    }
}

static FT_Glyph_Metrics *
pango_ft2_get_per_char (PangoFont *font,
			guint32    glyph_index)
{
  PangoFT2Font *ft2font = (PangoFT2Font *)font;
  FT_Face face;
	
  face = pango_ft2_font_get_face (font);
  
  FT_Load_Glyph (face, glyph_index, ft2font->load_flags);
  return &face->glyph->metrics;
}

static PangoFT2GlyphInfo *
pango_ft2_font_get_glyph_info (PangoFont   *font,
			       PangoGlyph   glyph,
			       gboolean     create)
{
  PangoFT2Font *ft2font = (PangoFT2Font *)font;
  PangoFT2GlyphInfo *info;
  FT_Glyph_Metrics *gm;

  info = g_hash_table_lookup (ft2font->glyph_info, GUINT_TO_POINTER (glyph));

  if ((info == NULL) && create)
    {
      FT_Face face = pango_ft2_font_get_face (font);
      info = g_new0 (PangoFT2GlyphInfo, 1);
      
      if (glyph && (gm = pango_ft2_get_per_char (font, glyph)))
	{
	  info->ink_rect.x = PANGO_UNITS_26_6 (gm->horiBearingX);
	  info->ink_rect.width = PANGO_UNITS_26_6 (gm->width);
	  info->ink_rect.y = -PANGO_UNITS_26_6 (gm->horiBearingY);
	  info->ink_rect.height = PANGO_UNITS_26_6 (gm->height);
	      
	  info->logical_rect.x = 0;
	  info->logical_rect.width = PANGO_UNITS_26_6 (gm->horiAdvance);
	  info->logical_rect.y = -PANGO_UNITS_26_6 (face->size->metrics.ascender + 64);
	  /* Some fonts report negative descender, some positive ! (?) */
	  info->logical_rect.height = PANGO_UNITS_26_6 (face->size->metrics.ascender + ABS (face->size->metrics.descender) + 128);
	}
      else
	{
	  info->ink_rect.x = 0;
	  info->ink_rect.width = 0;
	  info->ink_rect.y = 0;
	  info->ink_rect.height = 0;

	  info->logical_rect.x = 0;
	  info->logical_rect.width = 0;
	  info->logical_rect.y = 0;
	  info->logical_rect.height = 0;
	}

      g_hash_table_insert (ft2font->glyph_info, GUINT_TO_POINTER(glyph), info);
    }

  return info;
}
     
static void
pango_ft2_font_get_glyph_extents (PangoFont      *font,
				  PangoGlyph      glyph,
				  PangoRectangle *ink_rect,
				  PangoRectangle *logical_rect)
{
  PangoFT2GlyphInfo *info;

  info = pango_ft2_font_get_glyph_info (font, glyph, TRUE);

  if (ink_rect)
    *ink_rect = info->ink_rect;
  if (logical_rect)
    *logical_rect = info->logical_rect;
}

/**
 * pango_ft2_font_get_kerning:
 * @font: a #PangoFont
 * @left: the left #PangoGlyph
 * @right: the right #PangoGlyph
 * 
 * Retrieves kerning information for a combination of two glyphs.
 * 
 * Return value: The amount of kerning (in Pango units) to apply for 
 * the given combination of glyphs.
 **/
int
pango_ft2_font_get_kerning (PangoFont *font,
			    PangoGlyph left,
			    PangoGlyph right)
{
  FT_Face face;
  FT_Error error;
  FT_Vector kerning;

  face = pango_ft2_font_get_face (font);
  if (!face)
    return 0;

  if (!FT_HAS_KERNING (face))
    return 0;

  if (!left || !right)
    return 0;

  error = FT_Get_Kerning (face, left, right,
			  ft_kerning_default, &kerning);
  if (error != FT_Err_Ok)
    g_warning ("FT_Get_Kerning returns error: %s",
	       _pango_ft2_ft_strerror (error));

  return PANGO_UNITS_26_6 (kerning.x);
}

static PangoFontMetrics *
pango_ft2_font_get_metrics (PangoFont     *font,
			    PangoLanguage *language)
{
  PangoFT2Font *ft2font = PANGO_FT2_FONT (font);
  PangoFT2MetricsInfo *info = NULL; /* Quiet gcc */
  GSList *tmp_list;      

  const char *sample_str = pango_language_get_sample_string (language);
  
  tmp_list = ft2font->metrics_by_lang;
  while (tmp_list)
    {
      info = tmp_list->data;
      
      if (info->sample_str == sample_str)    /* We _don't_ need strcmp */
	break;

      tmp_list = tmp_list->next;
    }

  if (!tmp_list)
    {
      PangoContext *context;
      PangoLayout *layout;
      PangoRectangle extents;
      FT_Face face = pango_ft2_font_get_face (font);

      info = g_new (PangoFT2MetricsInfo, 1);
      info->sample_str = sample_str;
      info->metrics = pango_font_metrics_new ();

      info->metrics->ascent = PANGO_UNITS_26_6 (face->size->metrics.ascender);
      info->metrics->descent = PANGO_UNITS_26_6 (- face->size->metrics.descender);
      info->metrics->approximate_char_width = 
        info->metrics->approximate_digit_width = 
        PANGO_UNITS_26_6 (face->size->metrics.max_advance);

      ft2font->metrics_by_lang = g_slist_prepend (ft2font->metrics_by_lang, info);

      context = pango_context_new ();
      pango_context_set_font_map (context, ft2font->fontmap);
      pango_context_set_language (context, language);
      
      layout = pango_layout_new (context);
      pango_layout_set_font_description (layout, ft2font->description);

      pango_layout_set_text (layout, sample_str, -1);      
      pango_layout_get_extents (layout, NULL, &extents);
      
      info->metrics->approximate_char_width = 
        extents.width / g_utf8_strlen (sample_str, -1);

      pango_layout_set_text (layout, "0123456789", -1);      
      pango_layout_get_extents (layout, NULL, &extents);
      
      info->metrics->approximate_digit_width = extents.width / 10;

      g_object_unref (layout);
      g_object_unref (context);
    }

  return pango_font_metrics_ref (info->metrics);
}

static gboolean
pango_ft2_free_glyph_info_callback (gpointer key, gpointer value, gpointer data)
{
  PangoFT2Font *font = PANGO_FT2_FONT (data);
  PangoFT2GlyphInfo *info = value;
  
  if (font->glyph_cache_destroy && info->cached_glyph)
    (*font->glyph_cache_destroy) (info->cached_glyph);

  g_free (value);
  return TRUE;
}

static void
free_metrics_info (PangoFT2MetricsInfo *info)
{
  pango_font_metrics_unref (info->metrics);
  g_free (info);
}

static void
pango_ft2_font_finalize (GObject *object)
{
  PangoFT2Font *ft2font = (PangoFT2Font *)object;

  _pango_ft2_font_map_remove (ft2font->fontmap, ft2font);

  if (ft2font->face)
    {
      FT_Done_Face (ft2font->face);
      ft2font->face = NULL;
    }

  pango_font_description_free (ft2font->description);
  FcPatternDestroy (ft2font->font_pattern);
  
  g_object_unref (ft2font->fontmap);

  g_slist_foreach (ft2font->metrics_by_lang, (GFunc)free_metrics_info, NULL);
  g_slist_free (ft2font->metrics_by_lang);  

  g_hash_table_foreach_remove (ft2font->glyph_info,
			       pango_ft2_free_glyph_info_callback, object);
  g_hash_table_destroy (ft2font->glyph_info);
  
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static PangoFontDescription *
pango_ft2_font_describe (PangoFont *font)
{
  PangoFT2Font         *ft2font;
  PangoFontDescription *desc;

  ft2font = PANGO_FT2_FONT (font);

  desc = pango_font_description_copy (ft2font->description);

  return desc;
}

PangoMap *
pango_ft2_get_shaper_map (PangoLanguage *language)
{
  static guint engine_type_id = 0;
  static guint render_type_id = 0;
  
  if (engine_type_id == 0)
    {
      engine_type_id = g_quark_from_static_string (PANGO_ENGINE_TYPE_SHAPE);
      render_type_id = g_quark_from_static_string (PANGO_RENDER_TYPE_FT2);
    }
  
  return pango_find_map (language, engine_type_id, render_type_id);
}

/**
 * pango_ft2_font_get_coverage:
 * @font: a #PangoFT2Font.
 * @language: a language tag.
 * @returns: a #PangoCoverage.
 * 
 * Should not be called directly, use pango_font_get_coverage() instead.
 **/
PangoCoverage *
pango_ft2_font_get_coverage (PangoFont     *font,
			     PangoLanguage *language)
{
  PangoFT2Font *ft2font = (PangoFT2Font *)font;

  return _pango_ft2_font_map_get_coverage (ft2font->fontmap, ft2font->font_pattern);
}

static PangoEngineShape *
pango_ft2_font_find_shaper (PangoFont     *font,
			    PangoLanguage *language,
			    guint32        ch)
{
  PangoMap *shape_map = NULL;

  shape_map = pango_ft2_get_shaper_map (language);
  return (PangoEngineShape *)pango_map_get_engine (shape_map, ch);
}

/* Utility functions */

/**
 * pango_ft2_get_unknown_glyph:
 * @font: a #PangoFont
 * 
 * Return the index of a glyph suitable for drawing unknown characters.
 * 
 * Return value: a glyph index into @font
 **/
PangoGlyph
pango_ft2_get_unknown_glyph (PangoFont *font)
{
  return 0;
}


static void
pango_ft2_draw_hline (FT_Bitmap *bitmap,
		      int        y,
		      int        start,
		      int        end)
{
  unsigned char *p;
  int ix;

  if (y < 0 || y >= bitmap->rows)
    return;

  if (end <= 0 || start >= bitmap->width)
    return;
  
  if (start < 0)
    start = 0;

  if (end >= bitmap->width)
    end = bitmap->width;

  p = bitmap->buffer + y * bitmap->pitch + start;
	      
  for (ix = 0; ix < end - start; ix++)
    *p++ = 0xff;
}

/**
 * pango_ft2_render_layout_line:
 * @bitmap:    a FT_Bitmap to render the line onto
 * @line:      a #PangoLayoutLine
 * @x:         the x position of start of string (in pixels)
 * @y:         the y position of baseline (in pixels)
 *
 * Render a #PangoLayoutLine onto a FreeType2 bitmap
 */
void 
pango_ft2_render_layout_line (FT_Bitmap       *bitmap,
			      PangoLayoutLine *line,
			      int              x, 
			      int              y)
{
  GSList *tmp_list = line->runs;
  PangoRectangle logical_rect;
  PangoRectangle ink_rect;
  int x_off = 0;

  while (tmp_list)
    {
      PangoUnderline uline = PANGO_UNDERLINE_NONE;
      gboolean strike, shape_set;
      gint rise, risen_y;
      PangoLayoutRun *run = tmp_list->data;

      tmp_list = tmp_list->next;

      pango_ft2_get_item_properties (run->item,
				     &uline, &strike, &rise,
				     &shape_set, &ink_rect, &logical_rect);

      risen_y = y - PANGO_PIXELS (rise);

      if (!shape_set)
	{
	  if (uline == PANGO_UNDERLINE_NONE)
	    pango_glyph_string_extents (run->glyphs, run->item->analysis.font,
					NULL, &logical_rect);
	  else
	    pango_glyph_string_extents (run->glyphs, run->item->analysis.font,
					&ink_rect, &logical_rect);

	  pango_ft2_render (bitmap, run->item->analysis.font, run->glyphs,
			    x + PANGO_PIXELS (x_off), risen_y);
	}

      switch (uline)
	{
	case PANGO_UNDERLINE_NONE:
	  break;
	case PANGO_UNDERLINE_DOUBLE:
	  pango_ft2_draw_hline (bitmap,
				risen_y + 4,
				x + PANGO_PIXELS (x_off + ink_rect.x),
				x + PANGO_PIXELS (x_off + ink_rect.x + ink_rect.width));
	  /* Fall through */
	case PANGO_UNDERLINE_SINGLE:
	  pango_ft2_draw_hline (bitmap,
				risen_y + 2,
				x + PANGO_PIXELS (x_off + ink_rect.x),
				x + PANGO_PIXELS (x_off + ink_rect.x + ink_rect.width));
	  break;
	case PANGO_UNDERLINE_LOW:
	  pango_ft2_draw_hline (bitmap,
				risen_y + PANGO_PIXELS (ink_rect.y + ink_rect.height),
				x + PANGO_PIXELS (x_off + ink_rect.x),
				x + PANGO_PIXELS (x_off + ink_rect.x + ink_rect.width));
	  break;
	}

      if (strike)
	pango_ft2_draw_hline (bitmap,
			      risen_y + PANGO_PIXELS (logical_rect.y + logical_rect.height / 2),
			      x + PANGO_PIXELS (x_off + logical_rect.x),
			      x + PANGO_PIXELS (x_off + logical_rect.x + logical_rect.width));

      x_off += logical_rect.width;
    }
}

/**
 * pango_ft2_render_layout:
 * @bitmap:    a FT_Bitmap to render the layout onto
 * @layout:    a #PangoLayout
 * @x:         the X position of the left of the layout (in pixels)
 * @y:         the Y position of the top of the layout (in pixels)
 *
 * Render a #PangoLayout onto a FreeType2 bitmap
 */
void 
pango_ft2_render_layout (FT_Bitmap   *bitmap,
			 PangoLayout *layout,
			 int          x, 
			 int          y)
{
  PangoLayoutIter *iter;

  g_return_if_fail (bitmap != NULL);
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
      
      pango_ft2_render_layout_line (bitmap,
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
pango_ft2_get_item_properties (PangoItem      *item,
			       PangoUnderline *uline,
			       gboolean       *strikethrough,
                               gint           *rise,
			       gboolean       *shape_set,
			       PangoRectangle *ink_rect,
			       PangoRectangle *logical_rect)
{
  GSList *tmp_list = item->analysis.extra_attrs;

  if (strikethrough)
    *strikethrough = FALSE;

  if (rise)
    *rise = 0;

  if (shape_set)
    *shape_set = FALSE;

  while (tmp_list)
    {
      PangoAttribute *attr = tmp_list->data;

      switch (attr->klass->type)
	{
	case PANGO_ATTR_UNDERLINE:
	  if (uline)
	    *uline = ((PangoAttrInt *)attr)->value;
	  break;
	  
	case PANGO_ATTR_STRIKETHROUGH:
	  if (strikethrough)
	    *strikethrough = ((PangoAttrInt *)attr)->value;
	  break;

	case PANGO_ATTR_SHAPE:
	  if (shape_set)
	    *shape_set = TRUE;
	  if (logical_rect)
	    *logical_rect = ((PangoAttrShape *)attr)->logical_rect;
	  if (ink_rect)
	    *ink_rect = ((PangoAttrShape *)attr)->ink_rect;
	  break;

        case PANGO_ATTR_RISE:
          if (rise)
            *rise = ((PangoAttrInt *)attr)->value;
          break;

	default:
	  break;
	}

      tmp_list = tmp_list->next;
    }
}

typedef struct
{
  FT_Error     code;
  const char*  msg;
} ft_error_description;

static int
ft_error_compare (const void *pkey,
		  const void *pbase)
{
  return ((ft_error_description *) pkey)->code - ((ft_error_description *) pbase)->code;
}

const char *
_pango_ft2_ft_strerror (FT_Error error)
{
#undef __FTERRORS_H__
#define FT_ERRORDEF( e, v, s )  { e, s },
#define FT_ERROR_START_LIST  {
#define FT_ERROR_END_LIST    { 0, 0 } };

  const ft_error_description ft_errors[] =
#include FT_ERRORS_H

#undef FT_ERRORDEF
#undef FT_ERROR_START_LIST
#undef FT_ERROR_END_LIST

  ft_error_description *found =
    bsearch (&error, ft_errors, G_N_ELEMENTS (ft_errors) - 1,
	     sizeof (ft_errors[0]), ft_error_compare);
  if (found != NULL)
    return found->msg;
  else
    {
      static char default_msg[100];

      g_sprintf (default_msg, "Unknown FreeType2 error %#x", error);
      return default_msg;
    }
}


void *
pango_ft2_font_get_cache_glyph_data (PangoFont *font,
				     int        glyph_index)
{
  PangoFT2GlyphInfo *info;

  g_return_val_if_fail (PANGO_FT2_IS_FONT (font), NULL);
  
  info = pango_ft2_font_get_glyph_info (font, glyph_index, FALSE);

  if (info == NULL)
    return NULL;
  
  return info->cached_glyph;
}

void
pango_ft2_font_set_cache_glyph_data (PangoFont     *font,
				     int            glyph_index,
				     void          *cached_glyph)
{
  PangoFT2GlyphInfo *info;

  g_return_if_fail (PANGO_FT2_IS_FONT (font));
  
  info = pango_ft2_font_get_glyph_info (font, glyph_index, TRUE);

  info->cached_glyph = cached_glyph;

  /* TODO: Implement limiting of the number of cached glyphs */
}

void
pango_ft2_font_set_glyph_cache_destroy (PangoFont      *font,
					GDestroyNotify  destroy_notify)
{
  g_return_if_fail (PANGO_FT2_IS_FONT (font));
  
  PANGO_FT2_FONT (font)->glyph_cache_destroy = destroy_notify;
}
