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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
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

#ifndef __GTK_TYPE_UTILS_H__
#define __GTK_TYPE_UTILS_H__


#include <glib-object.h>


G_BEGIN_DECLS

#ifndef GTK_DISABLE_DEPRECATED

/* Fundamental Types
 * many of these are just aliases for GLib types to maintain
 * compatibility
 */

#define GTK_TYPE_INVALID G_TYPE_INVALID
#define GTK_TYPE_NONE    G_TYPE_NONE
#define GTK_TYPE_ENUM    G_TYPE_ENUM
#define GTK_TYPE_FLAGS   G_TYPE_FLAGS

/* GtkArg types */
#define GTK_TYPE_CHAR    G_TYPE_CHAR
#define GTK_TYPE_UCHAR   G_TYPE_UCHAR
#define GTK_TYPE_BOOL    G_TYPE_BOOLEAN
#define GTK_TYPE_INT     G_TYPE_INT
#define GTK_TYPE_UINT    G_TYPE_UINT
#define GTK_TYPE_LONG    G_TYPE_LONG
#define GTK_TYPE_ULONG   G_TYPE_ULONG
#define GTK_TYPE_FLOAT   G_TYPE_FLOAT
#define GTK_TYPE_DOUBLE  G_TYPE_DOUBLE
#define GTK_TYPE_STRING  G_TYPE_STRING
#define GTK_TYPE_BOXED   G_TYPE_BOXED
#define GTK_TYPE_POINTER G_TYPE_POINTER

typedef GType GtkFundamentalType;

/* --- type macros --- */
#define GTK_CLASS_NAME(class)		(g_type_name (G_TYPE_FROM_CLASS (class)))
#define GTK_CLASS_TYPE(class)		(G_TYPE_FROM_CLASS (class))
#define GTK_TYPE_IS_OBJECT(type)	(g_type_is_a ((type), GTK_TYPE_OBJECT))

/* outdated macros that really shouldn't e used anymore,
 * use the GLib type system instead
 */
#define	GTK_TYPE_FUNDAMENTAL_LAST        (G_TYPE_LAST_RESERVED_FUNDAMENTAL - 1)
#define	GTK_TYPE_FUNDAMENTAL_MAX         (G_TYPE_FUNDAMENTAL_MAX)

#define	GTK_FUNDAMENTAL_TYPE	G_TYPE_FUNDAMENTAL
#define GTK_STRUCT_OFFSET	G_STRUCT_OFFSET

#endif /* GTK_DISABLE_DEPRECATED */

/* glib macro wrappers (compatibility) */
#define	GTK_CHECK_CAST		G_TYPE_CHECK_INSTANCE_CAST
#define	GTK_CHECK_CLASS_CAST	G_TYPE_CHECK_CLASS_CAST
#define GTK_CHECK_GET_CLASS	G_TYPE_INSTANCE_GET_CLASS
#define	GTK_CHECK_TYPE		G_TYPE_CHECK_INSTANCE_TYPE
#define	GTK_CHECK_CLASS_TYPE	G_TYPE_CHECK_CLASS_TYPE

/* glib type wrappers (compatibility) */

typedef GType			GtkType;

#ifndef GTK_DISABLE_DEPRECATED

typedef GTypeInstance		GtkTypeObject;
typedef GTypeClass		GtkTypeClass;
typedef GBaseInitFunc		GtkClassInitFunc;
typedef GInstanceInitFunc	GtkObjectInitFunc;

#endif /* GTK_DISABLE_DEPRECATED */

G_END_DECLS

/* Builtin Types
 */
#include <gtk/gtktypebuiltins.h>

G_BEGIN_DECLS

/* urg */
#define GTK_TYPE_IDENTIFIER (gtk_identifier_get_type ())
GType gtk_identifier_get_type (void);

/* --- typedefs --- */
/* here we come with some necessary forward declarations for structures and
 * provide some fundamental function signatures
 */
typedef struct _GtkArg	       	     GtkArg;
typedef struct _GtkObject   	     GtkObject; /* object forward declaration */
typedef gboolean (*GtkFunction)	    (gpointer      data);
typedef void (*GtkDestroyNotify)    (gpointer      data);
typedef void (*GtkCallbackMarshal)  (GtkObject    *object,
				     gpointer      data,
				     guint         n_args,
				     GtkArg       *args);
typedef void (*GtkSignalFunc)       (void);
#define GTK_SIGNAL_FUNC(f)	    ((GtkSignalFunc) (f))

#ifndef GTK_DISABLE_DEPRECATED
typedef struct _GtkTypeInfo 	     GtkTypeInfo;
typedef GSignalCMarshaller          GtkSignalMarshaller;
#endif

#if !defined (GTK_DISABLE_DEPRECATED) || defined (GTK_COMPILATION)
/* Used by gtk_container_foreach_unmarshal () */

/* GtkArg, used to hold differently typed values */
struct _GtkArg
{
  GtkType type;
  gchar *name;
  
  /* this union only defines the required storage types for
   * the possibile values, thus there is no gint enum_data field,
   * because that would just be a mere alias for gint int_data.
   * use the GTK_VALUE_*() and GTK_RETLOC_*() macros to access
   * the discrete memebers.
   */
  union {
    /* flat values */
    gchar char_data;
    guchar uchar_data;
    gboolean bool_data;
    gint int_data;
    guint uint_data;
    glong long_data;
    gulong ulong_data;
    gfloat float_data;
    gdouble double_data;
    gchar *string_data;
    GtkObject *object_data;
    gpointer pointer_data;
    
    /* structured values */
    struct {
      GtkSignalFunc f;
      gpointer d;
    } signal_data;
  } d;
};

/* argument value access macros, these must not contain casts,
 * to allow the usage of these macros in combination with the
 * address operator, e.g. &GTK_VALUE_CHAR (*arg)
 */
#define GTK_VALUE_CHAR(a)	((a).d.char_data)
#define GTK_VALUE_UCHAR(a)	((a).d.uchar_data)
#define GTK_VALUE_BOOL(a)	((a).d.bool_data)
#define GTK_VALUE_INT(a)	((a).d.int_data)
#define GTK_VALUE_UINT(a)	((a).d.uint_data)
#define GTK_VALUE_LONG(a)	((a).d.long_data)
#define GTK_VALUE_ULONG(a)	((a).d.ulong_data)
#define GTK_VALUE_FLOAT(a)	((a).d.float_data)
#define GTK_VALUE_DOUBLE(a)	((a).d.double_data)
#define GTK_VALUE_STRING(a)	((a).d.string_data)
#define GTK_VALUE_ENUM(a)	((a).d.int_data)
#define GTK_VALUE_FLAGS(a)	((a).d.uint_data)
#define GTK_VALUE_BOXED(a)	((a).d.pointer_data)
#define GTK_VALUE_OBJECT(a)	((a).d.object_data)
#define GTK_VALUE_POINTER(a)	((a).d.pointer_data)
#define GTK_VALUE_SIGNAL(a)	((a).d.signal_data)
#endif /* !GTK_DISABLE_DEPRECATED || GTK_COMPILATION */

#ifndef GTK_DISABLE_DEPRECATED
/* return location macros, these all narrow down to
 * pointer types, because return values need to be
 * passed by reference
 */
#define GTK_RETLOC_CHAR(a)	((gchar*)	(a).d.pointer_data)
#define GTK_RETLOC_UCHAR(a)	((guchar*)	(a).d.pointer_data)
#define GTK_RETLOC_BOOL(a)	((gboolean*)	(a).d.pointer_data)
#define GTK_RETLOC_INT(a)	((gint*)	(a).d.pointer_data)
#define GTK_RETLOC_UINT(a)	((guint*)	(a).d.pointer_data)
#define GTK_RETLOC_LONG(a)	((glong*)	(a).d.pointer_data)
#define GTK_RETLOC_ULONG(a)	((gulong*)	(a).d.pointer_data)
#define GTK_RETLOC_FLOAT(a)	((gfloat*)	(a).d.pointer_data)
#define GTK_RETLOC_DOUBLE(a)	((gdouble*)	(a).d.pointer_data)
#define GTK_RETLOC_STRING(a)	((gchar**)	(a).d.pointer_data)
#define GTK_RETLOC_ENUM(a)	((gint*)	(a).d.pointer_data)
#define GTK_RETLOC_FLAGS(a)	((guint*)	(a).d.pointer_data)
#define GTK_RETLOC_BOXED(a)	((gpointer*)	(a).d.pointer_data)
#define GTK_RETLOC_OBJECT(a)	((GtkObject**)	(a).d.pointer_data)
#define GTK_RETLOC_POINTER(a)	((gpointer*)	(a).d.pointer_data)
/* GTK_RETLOC_SIGNAL() - no such thing */

/* type registration, it is recommended to use
 * g_type_register_static() or
 * g_type_register_dynamic() instead
 */
struct _GtkTypeInfo
{
  gchar			*type_name;
  guint			 object_size;
  guint			 class_size;
  GtkClassInitFunc	 class_init_func;
  GtkObjectInitFunc	 object_init_func;
  gpointer		 reserved_1;
  gpointer		 reserved_2;
  GtkClassInitFunc	 base_class_init_func;
};

#endif /* GTK_DISABLE_DEPRECATED */

gpointer	gtk_type_class	(GtkType	 type);

#ifndef GTK_DISABLE_DEPRECATED

GtkType		gtk_type_unique	(GtkType	   parent_type,
				 const GtkTypeInfo *gtkinfo);
gpointer	gtk_type_new	(GtkType	 type);

/* --- compatibility defines --- */
#define	gtk_type_name(type)		 g_type_name (type)
#define	gtk_type_from_name(name)	 g_type_from_name (name)
#define	gtk_type_parent(type)		 g_type_parent (type)
#define	gtk_type_is_a(type, is_a_type)	 g_type_is_a ((type), (is_a_type))

/* enum/flags compatibility functions, we strongly
 * recommend to use the glib enum/flags classes directly
 */
typedef GEnumValue  GtkEnumValue;
typedef GFlagsValue GtkFlagValue;
GtkEnumValue*	gtk_type_enum_get_values	(GtkType	 enum_type);
GtkFlagValue*	gtk_type_flags_get_values	(GtkType	 flags_type);
GtkEnumValue*	gtk_type_enum_find_value	(GtkType	 enum_type,
						 const gchar	*value_name);
GtkFlagValue*	gtk_type_flags_find_value	(GtkType	 flags_type,
						 const gchar	*value_name);

#endif /* GTK_DISABLE_DEPRECATED */

#if !defined (GTK_DISABLE_DEPRECATED) || defined (GTK_COMPILATION)
/* deprecated, use g_type_init() instead */
void		gtk_type_init	(GTypeDebugFlags debug_flags);
#endif /* !GTK_DISABLE_DEPRECATED || GTK_COMPILATION */

G_END_DECLS

#endif /* __GTK_TYPE_UTILS_H__ */
