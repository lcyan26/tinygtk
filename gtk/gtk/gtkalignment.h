/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

#ifndef __GTK_ALIGNMENT_H__
#define __GTK_ALIGNMENT_H__


#include <gdk/gdk.h>
#include <gtk/gtkbin.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define GTK_TYPE_ALIGNMENT                  (gtk_alignment_get_type ())
#define GTK_ALIGNMENT(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_ALIGNMENT, GtkAlignment))
#define GTK_ALIGNMENT_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_ALIGNMENT, GtkAlignmentClass))
#define GTK_IS_ALIGNMENT(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_ALIGNMENT))
#define GTK_IS_ALIGNMENT_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_ALIGNMENT))
#define GTK_ALIGNMENT_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_ALIGNMENT, GtkAlignmentClass))


typedef struct _GtkAlignment       GtkAlignment;
typedef struct _GtkAlignmentClass  GtkAlignmentClass;

struct _GtkAlignment
{
  GtkBin bin;

  gfloat xalign;
  gfloat yalign;
  gfloat xscale;
  gfloat yscale;
};

struct _GtkAlignmentClass
{
  GtkBinClass parent_class;
};


GType      gtk_alignment_get_type   (void) G_GNUC_CONST;
GtkWidget* gtk_alignment_new        (gfloat             xalign,
				     gfloat             yalign,
				     gfloat             xscale,
				     gfloat             yscale);
void       gtk_alignment_set        (GtkAlignment      *alignment,
				     gfloat             xalign,
				     gfloat             yalign,
				     gfloat             xscale,
				     gfloat             yscale);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_ALIGNMENT_H__ */
