/*
 * gtkimmoduleime
 * Copyright (C) 2003 Takuro Ashie
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * $Id: imime.c,v 1.3 2004/12/01 20:23:48 tml Exp $
 */

#include <gtk/gtkimmodule.h>
#include "gtkimcontextime.h"

#include <string.h>

static const GtkIMContextInfo ime_ime_info = {
  "ime",
  "Windows IME",
  "gtk+",
  "",
  "ja:ko:zh",
};

static const GtkIMContextInfo *info_list[] = {
  &ime_ime_info,
};

void
ime_im_module_init (GTypeModule * module)
{
  gtk_im_context_ime_register_type (module);
}

void
ime_im_module_exit (void)
{
}

void
ime_im_module_list (const GtkIMContextInfo *** contexts, int *n_contexts)
{
  *contexts = info_list;
  *n_contexts = G_N_ELEMENTS (info_list);
}

GtkIMContext *
ime_im_module_create (const gchar * context_id)
{
 // g_return_val_if_fail (context_id, NULL);

  if (!strcmp (context_id, "ime"))
    return g_object_new (GTK_TYPE_IM_CONTEXT_IME, NULL);
  else
    return NULL;
}
