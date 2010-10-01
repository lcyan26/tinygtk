/* Pango
 * modules.c:
 *
 * Copyright (C) 1999 Red Hat Software
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
#include <limits.h>
#include <errno.h>

#include <gmodule.h>

#include "pango-modules.h"
#include "pango-utils.h"

typedef struct _PangoMapInfo PangoMapInfo;
typedef struct _PangoEnginePair PangoEnginePair;
typedef struct _PangoSubmap PangoSubmap;

struct _PangoSubmap
{
  gboolean is_leaf;
  union {
    PangoMapEntry entry;
    PangoMapEntry *leaves;
  } d;
};

struct _PangoMap
{
  gint n_submaps;
  PangoSubmap *submaps;
};

struct _PangoMapInfo
{
  PangoLanguage *language;
  guint engine_type_id;
  guint render_type_id;
  PangoMap *map;
};

struct _PangoEnginePair
{
  PangoEngineInfo info;
  gboolean included;
  void *load_info;
  PangoEngine *engine;
};

static GList *maps = NULL;

static GSList *builtin_engines = NULL;
static GSList *registered_engines = NULL;
static GSList *dlloaded_engines = NULL;

static void build_map    (PangoMapInfo *info);
static void init_modules (void);

/**
 * pango_find_map:
 * @language: the language tag for which to find the map
 * @engine_type_id: the engine type for the map to find
 * @render_type_id: the render type for the map to find
 * 
 * Locate a #PangoMap for a particular engine type and render
 * type. The resulting map can be used to determine the engine
 * for each character.
 * 
 * Return value: 
 **/
PangoMap *
pango_find_map (PangoLanguage *language,
		guint          engine_type_id,
		guint          render_type_id)
{
  GList *tmp_list = maps;
  PangoMapInfo *map_info = NULL;
  gboolean found_earlier = FALSE;

  while (tmp_list)
    {
      map_info = tmp_list->data;
      if (map_info->engine_type_id == engine_type_id &&
	  map_info->render_type_id == render_type_id)
	{
	  if (map_info->language == language)
	    break;
	  else
	    found_earlier = TRUE;
	}

      tmp_list = tmp_list->next;
    }

  if (!tmp_list)
    {
      map_info = g_new (PangoMapInfo, 1);
      map_info->language = language;
      map_info->engine_type_id = engine_type_id;
      map_info->render_type_id = render_type_id;

      build_map (map_info);

      maps = g_list_prepend (maps, map_info);
    }
  else if (found_earlier)
    {
      /* Move the found map to the beginning of the list
       * for speed next time around if we had to do
       * any failing comparison. (No longer so important,
       * since we don't strcmp.)
       */
      maps = g_list_remove_link(maps, tmp_list);
      maps = g_list_prepend(maps, tmp_list->data);
      g_list_free_1(tmp_list);
    }
  
  return map_info->map;
}

static PangoEngine *
pango_engine_pair_get_engine (PangoEnginePair *pair)
{
  if (!pair->engine)
    {
      if (pair->included)
	{
	  PangoIncludedModule *included_module = pair->load_info;
	  
	  pair->engine = included_module->load (pair->info.id);
	}
      else
	{
#if 0
	  GModule *module;
	  char *module_name = pair->load_info;
	  PangoEngine *(*load) (const gchar *id);
  	  
	  module = g_module_open (module_name, 0);
	  if (!module)
	    {
	      g_printerr ("Cannot load module %s: %s\n",
			  module_name, g_module_error());
	      return NULL;
	    }
	  
	  g_module_symbol (module, "script_engine_load", (gpointer *) &load);
	  if (!load)
	    {
	      g_printerr ("cannot retrieve script_engine_load from %s: %s\n",
			  module_name, g_module_error());
	      g_module_close (module);
	      return NULL;
	    }
	  
	  pair->engine = (*load) (pair->info.id);
#endif
	}
      
    }

  return pair->engine;
}

static void
handle_included_module (PangoIncludedModule *module,
			GSList              **engine_list)
{
  PangoEngineInfo *engine_info;
  int n_engines;
  int i;

  module->list (&engine_info, &n_engines);

  for (i = 0; i < n_engines; i++)
    {
      PangoEnginePair *pair = g_new (PangoEnginePair, 1);

      pair->info = engine_info[i];
      pair->included = TRUE;
      pair->load_info = module;
      pair->engine = NULL;
      
      *engine_list = g_slist_prepend (*engine_list, pair);
    }
}

static gboolean /* Returns true if succeeded, false if failed */
process_module_file (FILE *module_file)
{
  GString *line_buf = g_string_new (NULL);
  GString *tmp_buf = g_string_new (NULL);
  gboolean have_error = FALSE;

  while (pango_read_line (module_file, line_buf))
    {
      PangoEnginePair *pair = g_new (PangoEnginePair, 1);
      PangoEngineRange *range;
      GList *ranges = NULL;
      GList *tmp_list;

      const char *p, *q;
      int i;
      int start, end;

      pair->included = FALSE;
      
      p = line_buf->str;

      if (!pango_skip_space (&p))
	{
	  g_free (pair);
	  continue;
	}

      i = 0;
      while (1)
	{
	  if (!pango_scan_string (&p, tmp_buf))
	    {
	      have_error = TRUE;
	      goto error;
	    }

	  switch (i)
	    {
	    case 0:
	      pair->load_info = g_strdup (tmp_buf->str);
#if defined(G_OS_WIN32) && defined(LIBDIR)
	      if (strncmp (pair->load_info,
			   LIBDIR "/pango/" MODULE_VERSION "/modules/",
			   strlen (LIBDIR "/pango/" MODULE_VERSION "/modules/")) == 0)
		{
		  /* This is an entry put there by make install on the
		   * packager's system. On Windows a prebuilt Pango
		   * package can be installed in a random
		   * location. The pango.modules file distributed in
		   * such a package contains paths from the package
		   * builder's machine. Replace the path with the real
		   * one on this machine. */
		  gchar *tem = pair->load_info;
		  pair->load_info =
		    g_strconcat (pango_get_lib_subdirectory (),
				 "\\" MODULE_VERSION "\\modules\\",
				 tem + strlen (LIBDIR "/pango/" MODULE_VERSION "/modules/"),
				 NULL);
		  g_free (tem);
		}

#endif
	      break;
	    case 1:
	      pair->info.id = g_strdup (tmp_buf->str);
	      break;
	    case 2:
	      pair->info.engine_type = g_strdup (tmp_buf->str);
	      break;
	    case 3:
	      pair->info.render_type = g_strdup (tmp_buf->str);
	      break;
	    default:
	      range = g_new (PangoEngineRange, 1);
	      if (sscanf(tmp_buf->str, "%d-%d:", &start, &end) != 2)
		{
		  g_printerr ("Error reading modules file");
		  have_error = TRUE;
		  goto error;
		}
	      q = strchr (tmp_buf->str, ':');
	      if (!q)
		{
		  g_printerr ( "Error reading modules file");
		  have_error = TRUE;
		  goto error;
		}
	      q++;
	      range->start = start;
	      range->end = end;
	      range->langs = g_strdup (q);
	      
	      ranges = g_list_prepend (ranges, range);
	    }

	  if (!pango_skip_space (&p))
	    break;
	  
	  i++;
	}
      
      if (i<3)
	{
	  g_printerr ("Error reading modules file");
	  have_error = TRUE;
	  goto error;
	}
      
      ranges = g_list_reverse (ranges);
      pair->info.n_ranges = g_list_length (ranges);
      pair->info.ranges = g_new (PangoEngineRange, pair->info.n_ranges);
      
      tmp_list = ranges;
      for (i=0; i<pair->info.n_ranges; i++)
	{
	  pair->info.ranges[i] = *(PangoEngineRange *)tmp_list->data;
	  tmp_list = tmp_list->next;
	}

      pair->engine = NULL;
      
      dlloaded_engines = g_slist_prepend (dlloaded_engines, pair);

    error:
      g_list_foreach (ranges, (GFunc)g_free, NULL);
      g_list_free (ranges);

      if (have_error)
	{
	  g_free(pair);
	  break;
	}
    }

  g_string_free (line_buf, TRUE);
  g_string_free (tmp_buf, TRUE);

  return !have_error;
}

#if 0

static void
read_modules (void)
{
  FILE *module_file;

  char *file_str =  pango_config_key_get ("Pango/ModuleFiles");
  char **files;
  int n;

  if (!file_str)
    file_str = g_build_filename (pango_get_sysconf_subdirectory (),
				 "pango.modules",
				 NULL);

  files = pango_split_file_list (file_str);

  n = 0;
  while (files[n])
    n++;

  while (n-- > 0)
    {
      module_file = fopen (files[n], "r");
      if (module_file)
	{
	  process_module_file(module_file);
	  fclose(module_file);
	}
    }

  g_strfreev (files);
  g_free (file_str);

  dlloaded_engines = g_slist_reverse (dlloaded_engines);
}
#endif

static void
set_entry (PangoMapEntry *entry, gboolean is_exact, PangoEngineInfo *info)
{
  if ((is_exact && !entry->is_exact) ||
      !entry->info)
    {
      entry->is_exact = is_exact;
      entry->info = info;
    }
}

static void
init_modules (void)
{
/*  static gboolean init = FALSE;

  if (init)
    return;
  else
    init = TRUE;
  
  read_modules ();*/
}

static PangoSubmap *
map_get_submap (PangoMap *map,
		int       index)
{
  if (index >= map->n_submaps)
    {
      /* Round up to a multiple of 256 */
      int new_n_submaps = (index + 0x100) & ~0xff;
      int i;
      
      map->submaps = g_renew (PangoSubmap, map->submaps, new_n_submaps);
      for (i=map->n_submaps; i<new_n_submaps; i++)
	{
	  map->submaps[i].is_leaf = TRUE;
	  map->submaps[i].d.entry.info = NULL;
	  map->submaps[i].d.entry.is_exact = FALSE;
	}
      
      map->n_submaps = new_n_submaps;
    }

  return &map->submaps[index];
}

static void
map_add_engine (PangoMapInfo    *info,
		PangoEnginePair *pair)
{
  int submap;
  int i, j;
  PangoMap *map = info->map;
 
  for (i=0; i<pair->info.n_ranges; i++)
    {
      gboolean is_exact = FALSE;

      if (pair->info.ranges[i].langs)
	{
	  if (pango_language_matches (info->language, pair->info.ranges[i].langs))
	    is_exact = TRUE;
	}
      
      for (submap = pair->info.ranges[i].start / 256;
	   submap <= pair->info.ranges[i].end / 256;
	   submap ++)
	{
	  PangoSubmap *submap_struct = map_get_submap (map, submap);
	  gunichar start;
	  gunichar end;

	  if (submap == pair->info.ranges[i].start / 256)
	    start = pair->info.ranges[i].start % 256;
	  else
	    start = 0;
	  
	  if (submap == pair->info.ranges[i].end / 256)
	    end = pair->info.ranges[i].end % 256;
	  else
	    end = 255;
	  
	  if (submap_struct->is_leaf &&
	      start == 0 && end == 255)
	    {
	      set_entry (&submap_struct->d.entry,
			 is_exact, &pair->info);
	    }
	  else
	    {
	      if (submap_struct->is_leaf)
		{
		  PangoMapEntry old_entry = submap_struct->d.entry;
		  
		  submap_struct->is_leaf = FALSE;
		  submap_struct->d.leaves = g_new (PangoMapEntry, 256);
		  for (j=0; j<256; j++)
		    submap_struct->d.leaves[j] = old_entry;
		}
	      
	      for (j=start; j<=end; j++)
		set_entry (&submap_struct->d.leaves[j],
			   is_exact, &pair->info);
	      
	    }
	}
    }
}

static void
map_add_engine_list (PangoMapInfo *info,
		     GSList       *engines,
		     const char   *engine_type,
		     const char   *render_type)  
{
  GSList *tmp_list = engines;

  while (tmp_list)
    {
      PangoEnginePair *pair = tmp_list->data;
      tmp_list = tmp_list->next;

      if (strcmp (pair->info.engine_type, engine_type) == 0 &&
	  strcmp (pair->info.render_type, render_type) == 0)
	{
	  map_add_engine (info, pair);
	}
    }
}

static void
build_map (PangoMapInfo *info)
{
  PangoMap *map;

  const char *engine_type = g_quark_to_string (info->engine_type_id);
  const char *render_type = g_quark_to_string (info->render_type_id);
  
  init_modules();

#if 0
  if (!dlloaded_engines && !registered_engines && !builtin_engines)
    {
      static gboolean no_module_warning = FALSE;
      if (!no_module_warning)
	{
	  gchar *filename = g_build_filename (pango_get_sysconf_subdirectory (),
					      "pango.modules",
					      NULL);
	  g_warning ("No builtin or dynamically loaded modules\n"
		     "were found. Pango will not work correctly. This probably means\n"
		     "there was an error in the creation of:\n"
		     "  '%s'\n"
		     "You may be able to recreate this file by running pango-querymodules.",
		     filename);
	  g_free (filename);
	  
	  no_module_warning = TRUE;
	}
    }
#endif
  
  info->map = map = g_new (PangoMap, 1);
  map->submaps = NULL;
  map->n_submaps = 0;

  map_add_engine_list (info, dlloaded_engines, engine_type, render_type);  
  map_add_engine_list (info, registered_engines, engine_type, render_type);  
  map_add_engine_list (info, builtin_engines, engine_type, render_type);  
}

/**
 * pango_map_get_entry:
 * @map: a #PangoMap
 * @wc:  an ISO-10646 codepoint
 * 
 * Returns the entry in the map for a given codepoint. The entry
 * contains information about the engine that should be used for
 * the codepoint and also whether the engine matches the language
 * tag for which the map was created exactly or just approximately.
 * 
 * Return value: the #PangoMapEntry for the codepoint. This value
 *   is owned by the #PangoMap and should not be freed.
 **/
PangoMapEntry *
pango_map_get_entry (PangoMap   *map,
		     guint32     wc)
{
  int i = wc / 256;

  if (i < map->n_submaps)
    {
      PangoSubmap *submap = &map->submaps[i];
      return submap->is_leaf ? &submap->d.entry : &submap->d.leaves[wc % 256];
    }
  else
    {
      static PangoMapEntry default_entry = { NULL, FALSE };
      return &default_entry;
    }
}

/**
 * pango_map_get_engine:
 * @map: a #PangoMap
 * @wc:  an ISO-10646 codepoint
 * 
 * Returns the engine listed in the map for a given codepoint. 
 * 
 * Return value: the engine, if one is listed for the codepoint,
 *    or %NULL. The lookup may cause the engine to be loaded;
 *    once an engine is loaded
 **/
PangoEngine *
pango_map_get_engine (PangoMap *map,
		      guint32   wc)
{
  int i = wc / 256;

  if (i < map->n_submaps)
    {
      PangoSubmap *submap = &map->submaps[i];
      PangoMapEntry *entry = submap->is_leaf ? &submap->d.entry : &submap->d.leaves[wc % 256];
      
      if (entry->info)
	return pango_engine_pair_get_engine ((PangoEnginePair *)entry->info);
      else
	return NULL;
    }
  else
    return NULL;
}

/**
 * pango_module_register:
 * @module: a #PangoIncludedModule
 * 
 * Registers a statically linked module with Pango. The
 * #PangoIncludedModule structure that is passed in contains the
 * functions that would otherwise be loaded from a dynamically loaded
 * module.
 **/
void
pango_module_register (PangoIncludedModule *module)
{
  GSList *tmp_list = NULL;
  
  handle_included_module (module, &tmp_list);

  registered_engines = g_slist_concat (registered_engines,
				       g_slist_reverse (tmp_list)); 
}
