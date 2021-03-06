# Makefile.am for gtk+/gtk

SUBDIRS=stock-icons theme-bits

INCLUDES = @STRIP_BEGIN@ \
	-DG_LOG_DOMAIN=\"Gtk\"				\
	-DGTK_LIBDIR=\"$(libdir)\"			\
	-DGTK_DATA_PREFIX=\"$(prefix)\"			\
	-DGTK_SYSCONFDIR=\"$(sysconfdir)\"		\
	-DGTK_VERSION=\"@GTK_VERSION@\"			\
	-DGTK_BINARY_VERSION=\"@GTK_BINARY_VERSION@\"	\
	-DGTK_HOST=\"@host@\"	                        \
	-DGTK_COMPILATION				\
	-I$(top_builddir)/gtk				\
	-I$(top_srcdir) -I../gdk			\
	-I$(top_srcdir)/gdk				\
	-I$(top_srcdir)/gdk-pixbuf -I../gdk-pixbuf	\
	-DG_DISABLE_DEPRECATED				\
	-DGDK_PIXBUF_DISABLE_DEPRECATED			\
	-DGDK_DISABLE_DEPRECATED			\
	-DGTK_DISABLE_DEPRECATED			\
	@GTK_DEBUG_FLAGS@				\
	@GTK_DEP_CFLAGS@				\
@STRIP_END@

gtarget=@gdktarget@

if PLATFORM_WIN32
no_undefined = -no-undefined
endif

if OS_WIN32
gtk_def = gtk.def
gtk_win32_symbols = -export-symbols $(gtk_def)

gtk_win32res_lo = gtk-win32res.lo

gtk-win32res.lo : gtk-win32.rc
	$(top_srcdir)/build/win32/lt-compile-resource gtk-win32.rc gtk-win32res.lo

install-libtool-import-lib:
	$(INSTALL) .libs/libgtk-win32-$(GTK_API_VERSION).dll.a $(DESTDIR)$(libdir)
uninstall-libtool-import-lib:
	-rm $(DESTDIR)$(libdir)/libtk-win32-$(GTK_API_VERSION).dll.a
else
install-libtool-import-lib:
uninstall-libtool-import-lib:
endif

if MS_LIB_AVAILABLE
noinst_DATA = gtk-win32-$(GTK_API_VERSION).lib

gtk-win32-$(GTK_API_VERSION).lib: libgtk-win32-$(GTK_API_VERSION).la gtk.def
	lib -name:libgtk-win32-$(GTK_API_VERSION)-@LT_CURRENT_MINUS_AGE@.dll -def:gtk.def -out:$@

install-ms-lib:
	$(INSTALL) gtk-win32-$(GTK_API_VERSION).lib $(DESTDIR)$(libdir)

uninstall-ms-lib:
	-rm $(DESTDIR)$(libdir)/gtk-win32-$(GTK_API_VERSION).lib
else
install-ms-lib:
uninstall-ms-lib:
endif

# libtool stuff: set version and export symbols for resolving
# since automake doesn't support conditionalized libsomething_la_LDFLAGS
# we use the general approach here
libgtkincludedir = $(includedir)/gtk-2.0/gtk
LDFLAGS = @STRIP_BEGIN@ \
	@LDFLAGS@								\
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)			\
	-export-dynamic								\
	-rpath $(libdir)							\
	$(no_undefined)								\
	@LIBTOOL_EXPORT_OPTIONS@						\
	$(top_builddir)/gdk-pixbuf/libgdk_pixbuf-$(GTK_API_VERSION).la		\
	$(top_builddir)/gdk/@gdktargetlib@					\
	$(libgtk_target_ldflags)						\
	@GTK_DEP_LIBS@								\
@STRIP_END@

#
# setup source file variables
#

# GTK+ header files for public installation (non-generated, or generated
# by configure)
gtk_public_h_sources = @STRIP_BEGIN@ \
	gtk.h			\
	gtkaccelgroup.h		\
	gtkaccellabel.h		\
	gtkaccelmap.h		\
	gtkaccessible.h		\
	gtkadjustment.h		\
	gtkalignment.h		\
	gtkarrow.h		\
	gtkaspectframe.h	\
	gtkbin.h		\
	gtkbindings.h		\
	gtkbbox.h		\
	gtkbox.h		\
	gtkbutton.h		\
	gtkcalendar.h		\
	gtkcheckbutton.h	\
	gtkcheckmenuitem.h	\
	gtkseparatormenuitem.h	\
	gtkclist.h		\
	gtkclipboard.h		\
	gtkcolorsel.h		\
	gtkcolorseldialog.h	\
	gtkcombo.h		\
	gtkcontainer.h		\
	gtkctree.h		\
	gtkcurve.h		\
	gtkcellrenderer.h	\
	gtkcelleditable.h	\
	gtkcellrenderertext.h	\
	gtkcellrenderertoggle.h	\
	gtkcellrendererpixbuf.h	\
	gtkdebug.h              \
	gtkdialog.h		\
	gtkdnd.h		\
	gtkdrawingarea.h	\
	gtkeditable.h           \
	gtkentry.h		\
	gtkenums.h		\
	gtkeventbox.h		\
	gtkfilesel.h		\
	gtkfixed.h		\
	gtkfontsel.h		\
	gtkframe.h		\
	gtkgamma.h		\
	gtkgc.h			\
	gtkhandlebox.h		\
	gtkhbbox.h		\
	gtkhbox.h		\
	gtkhpaned.h		\
	gtkhruler.h		\
	gtkhscale.h		\
	gtkhscrollbar.h		\
	gtkhseparator.h		\
	gtkiconfactory.h	\
	gtkimage.h		\
	gtkimagemenuitem.h	\
	gtkimcontext.h		\
	gtkimcontextsimple.h	\
	gtkimmodule.h		\
	gtkimmulticontext.h	\
	gtkinputdialog.h	\
	gtkinvisible.h		\
	gtkitem.h		\
	gtkitemfactory.h	\
	gtklabel.h		\
	gtklayout.h             \
	gtklist.h		\
	gtklistitem.h		\
	gtkliststore.h		\
	gtkmain.h		\
	gtkmenu.h		\
	gtkmenubar.h		\
	gtkmenuitem.h		\
	gtkmenushell.h		\
	gtkmessagedialog.h	\
	gtkmisc.h		\
	gtknotebook.h		\
	gtkoldeditable.h	\
	gtkobject.h		\
	gtkoptionmenu.h		\
	gtkpaned.h		\
	gtkpixmap.h		\
	gtkplug.h		\
	gtkpreview.h		\
	gtkprivate.h		\
	gtkprogress.h		\
	gtkprogressbar.h	\
	gtkradiobutton.h	\
	gtkradiomenuitem.h	\
	gtkrange.h		\
	gtkrc.h			\
	gtkruler.h		\
	gtkscale.h		\
	gtkscrollbar.h		\
	gtkscrolledwindow.h	\
	gtkselection.h		\
	gtkseparator.h		\
	gtksettings.h		\
	gtksignal.h		\
	gtksizegroup.h		\
	gtksocket.h		\
	gtkspinbutton.h		\
	gtkstyle.h		\
	gtkstatusbar.h		\
	gtkstock.h		\
	gtktable.h		\
	gtktearoffmenuitem.h    \
	gtktextbuffer.h		\
	gtktextchild.h		\
	gtktextdisplay.h	\
	gtktextiter.h		\
	gtktextlayout.h		\
	gtktextmark.h		\
	gtktexttag.h		\
	gtktexttagtable.h	\
	gtktextview.h		\
	gtktext.h		\
	gtktipsquery.h		\
	gtktogglebutton.h	\
	gtktoolbar.h		\
	gtktooltips.h		\
	gtktree.h		\
	gtktreednd.h		\
	gtktreeitem.h		\
	gtktreemodel.h		\
	gtktreemodelsort.h	\
	gtktreeselection.h	\
	gtktreesortable.h	\
	gtktreestore.h		\
	gtktreeview.h		\
	gtktreeviewcolumn.h	\
	gtktypeutils.h		\
	gtkvbbox.h		\
	gtkvbox.h		\
	gtkviewport.h		\
	gtkvpaned.h		\
	gtkvruler.h		\
	gtkvscale.h		\
	gtkvscrollbar.h		\
	gtkvseparator.h		\
	gtkwidget.h		\
	gtkwindow.h		\
@STRIP_END@
# GTK+ header files that don't get installed
gtk_private_h_sources = @STRIP_BEGIN@ \
	gtkrbtree.h 		\
	gtktextbtree.h		\
	gtktextchildprivate.h   \
	gtktextsegment.h	\
	gtktexttypes.h		\
	gtktextutil.h		\
	gtktextiterprivate.h	\
	gtktextmarkprivate.h	\
	gtktexttagprivate.h	\
	gtkthemes.h		\
	gtktreedatalist.h	\
	gtktreeprivate.h	\
	gtkwindow-decorate.h    \
@STRIP_END@
# GTK+ C sources to build the library from
gtk_c_sources = @STRIP_BEGIN@   \
	gtkaccelgroup.c		\
	gtkaccelmap.c		\
	gtkaccellabel.c		\
	gtkaccessible.c		\
	gtkadjustment.c		\
	gtkalignment.c		\
	gtkarrow.c		\
	gtkaspectframe.c	\
	gtkbin.c		\
	gtkbindings.c		\
	gtkbbox.c		\
	gtkbox.c		\
	gtkbutton.c		\
	gtkcalendar.c		\
	gtkcellrenderer.c	\
	gtkcelleditable.c	\
	gtkcellrenderertext.c	\
	gtkcellrenderertoggle.c	\
	gtkcellrendererpixbuf.c	\
	gtkcheckbutton.c	\
	gtkcheckmenuitem.c	\
	gtkseparatormenuitem.c	\
	gtkclipboard.c		\
	gtkclist.c		\
	gtkcolorsel.c		\
	gtkcolorseldialog.c	\
	gtkcombo.c		\
	gtkcontainer.c		\
	gtkctree.c		\
	gtkcurve.c		\
	gtkdialog.c		\
	gtkdnd.c		\
	gtkdrawingarea.c	\
	gtkeditable.c           \
	gtkentry.c		\
	gtkeventbox.c		\
	gtkfilesel.c		\
	gtkfixed.c		\
	gtkfontsel.c            \
	gtkframe.c		\
	gtkgamma.c		\
	gtkgc.c			\
	gtkhandlebox.c		\
	gtkhbbox.c		\
	gtkhbox.c		\
	gtkhpaned.c		\
	gtkhruler.c		\
	gtkhscale.c		\
	gtkhscrollbar.c		\
	gtkhseparator.c		\
	gtkhsv.c		\
	gtkhsv.h		\
	gtkiconfactory.c	\
	gtkimage.c		\
	gtkimagemenuitem.c	\
	gtkimcontext.c		\
	gtkimcontextsimple.c	\
	gtkimmodule.c		\
	gtkimmulticontext.c	\
	gtkinputdialog.c	\
	gtkintl.h		\
	gtkinvisible.c		\
	gtkitem.c		\
	gtkitemfactory.c	\
	gtkkeyhash.c		\
	gtkkeyhash.h		\
	gtklabel.c		\
	gtklayout.c		\
	gtklist.c		\
	gtklistitem.c		\
	gtkliststore.c		\
	gtkmain.c		\
	gtkmarshalers.c		\
	gtkmarshal.c		\
	gtkmenu.c		\
	gtkmenubar.c		\
	gtkmenuitem.c		\
	gtkmenushell.c		\
	gtkmessagedialog.c	\
	gtkmisc.c		\
	gtknotebook.c		\
	gtkobject.c		\
	gtkoldeditable.c	\
	gtkoptionmenu.c		\
	gtkpaned.c		\
	gtkpixmap.c		\
	gtkpreview.c		\
	gtkprogress.c		\
	gtkprogressbar.c	\
	gtkradiobutton.c	\
	gtkradiomenuitem.c	\
	gtkrange.c		\
	gtkrbtree.c 		\
	gtkrc.c			\
	gtkruler.c		\
	gtkscale.c		\
	gtkscrollbar.c		\
	gtkscrolledwindow.c	\
	gtkselection.c		\
	gtkseparator.c		\
	gtksettings.c		\
	gtksignal.c		\
	gtksizegroup.c		\
	gtkspinbutton.c		\
	gtkstock.c		\
	gtkstyle.c		\
	gtkstatusbar.c		\
	gtktable.c		\
	gtktearoffmenuitem.c    \
	gtktext.c		\
	gtktextbtree.c		\
	gtktextbuffer.c		\
	gtktextchild.c		\
	gtktextdisplay.c	\
	gtktextiter.c		\
	gtktextiterprivate.h	\
	gtktextlayout.c		\
	gtktextmark.c		\
	gtktextmarkprivate.h	\
	gtktextsegment.c	\
	gtktexttag.c		\
	gtktexttagtable.c	\
	gtktexttypes.c		\
	gtktextutil.c		\
	gtktextview.c		\
	gtkthemes.c		\
	gtktipsquery.c		\
	gtktogglebutton.c	\
	gtktoolbar.c		\
	gtktooltips.c		\
	gtktree.c		\
	gtktreeitem.c		\
	gtktreedatalist.c	\
	gtktreednd.c		\
	gtktreemodel.c		\
	gtktreemodelsort.c	\
	gtktreeselection.c	\
	gtktreesortable.c	\
	gtktreestore.c		\
	gtktreeview.c		\
	gtktreeviewcolumn.c	\
	gtktypebuiltins.c	\
	gtktypeutils.c		\
	gtkvbbox.c		\
	gtkvbox.c		\
	gtkviewport.c		\
	gtkvpaned.c		\
	gtkvruler.c		\
	gtkvscale.c		\
	gtkvscrollbar.c		\
	gtkvseparator.c		\
	gtkwidget.c		\
	gtkwindow.c		\
	gtkwindow-decorate.c    \
	fnmatch.c		\
	xembed.h		\
@STRIP_END@

gtk_plug_c_sources = @STRIP_BEGIN@   \
	gtkplug.c		\
	gtksocket.c		\
	gtkxembed.c		\
	gtkxembed.h		\
@STRIP_END@

# we use our own built_sources variable rules to avoid automake's
# BUILT_SOURCES oddities
# we generate frequently rebuild files piggyback on a stamp file, so sources
# depending on them only get rebuild when the built source actually changed
# content

# built sources that get installed with the header files
gtk_built_public_sources = @STRIP_BEGIN@ \
	gtkmarshal.h		\
	gtktypebuiltins.h	\
@STRIP_END@
gtk_built_private_headers = @STRIP_BEGIN@ \
	gtkmarshalers.h		\
@STRIP_END@
# built sources that don't get installed
gtk_built_sources = @STRIP_BEGIN@		\
	gtktypebuiltins.c			\
	gtkmarshalers.c				\
	gtkmarshalers.h				\
	gtkmarshal.c				\
	${gtk_built_private_headers}		\
	${gtk_built_public_sources}		\
@STRIP_END@
stamp_files = @STRIP_BEGIN@ 		\
	stamp-gtkmarshalers.h		\
	stamp-gtkmarshal.h		\
	stamp-gtktypebuiltins.h		\
@STRIP_END@
# non-header sources (headers should be specified in the above variables)
# that don't serve as direct make target sources, i.e. they don't have
# their own .lo rules and don't get publically installed
gtk_extra_sources = @STRIP_BEGIN@ \
	gtkversion.h.in		\
	gtkmarshalers.list	\
	gtkmarshal.list		\
@STRIP_END@

#
# setup GTK+ sources and their dependencies
#
MAINTAINERCLEANFILES = $(gtk_built_sources) $(stamp_files)
EXTRA_HEADERS =
EXTRA_DIST = $(gtk_private_h_sources) $(gtk_extra_sources)
EXTRA_DIST += $(gtk_built_sources)

#
# rules to generate built sources
#
# setup autogeneration dependencies
gen_sources = xgen-gdef xgen-gtbh xgen-gtic xgen-gmh xgen-gmc xgen-gmlh xgen-gmlc
CLEANFILES = $(gen_sources)

## automake 1.5 supports this without $(OBJECTS): $(gtk_built_sources) hack
#BUILT_SOURCES = $(gtk_built_sources)

$(libgtk_x11_2_0_la_OBJECTS) $(libgtk_linux_fb_2_0_la_OBJECTS) $(libgtk_win32_2_0_la_OBJECTS): ${gtk_built_public_sources} ${gtk_built_private_headers}

# all autogenerated files need to be generated in the srcdir,
# so old versions get remade and are not confused with newer
# versions in the build dir. thus a development setup requires
# srcdir to be writable, passing --disable-rebuilds to
# ../configure will supress all autogeneration rules.
$(srcdir)/gtkmarshalers.h: stamp-gtkmarshalers.h
	@true
stamp-gtkmarshalers.h: @REBUILD@ gtkmarshalers.list
	case @GLIB_GENMARSHAL@ in								\
	   .*) glib_genmarshal=`pwd`/@GLIB_GENMARSHAL@ ;;					\
	   *) glib_genmarshal=@GLIB_GENMARSHAL@ ;;						\
	esac;											\
	cd $(srcdir)										\
	&& $$glib_genmarshal --prefix=_gtk_marshal gtkmarshalers.list --header >> xgen-gmlh	\
	&& (cmp -s xgen-gmlh gtkmarshalers.h || cp xgen-gmlh gtkmarshalers.h)			\
	&& rm -f xgen-gmlh 									\
	&& echo timestamp > $(@F)	
$(srcdir)/gtkmarshalers.c: @REBUILD@ gtkmarshalers.list
	case @GLIB_GENMARSHAL@ in								\
	   .*) glib_genmarshal=`pwd`/@GLIB_GENMARSHAL@ ;;					\
	   *) glib_genmarshal=@GLIB_GENMARSHAL@ ;;						\
	esac;											\
	cd $(srcdir)										\
	&& $$glib_genmarshal --prefix=_gtk_marshal gtkmarshalers.list --body >> xgen-gmlc	\
	&& cp xgen-gmlc gtkmarshalers.c								\
	&& rm -f xgen-gmlc

$(srcdir)/gtkmarshal.h: stamp-gtkmarshal.h
	@true
stamp-gtkmarshal.h: @REBUILD@ gtkmarshal.list
	case @GLIB_GENMARSHAL@ in								\
	   .*) glib_genmarshal=`pwd`/@GLIB_GENMARSHAL@ ;;					\
	   *) glib_genmarshal=@GLIB_GENMARSHAL@ ;;						\
	esac;											\
	cd $(srcdir)										\
	&& echo "#ifndef GTK_DISABLE_DEPRECATED" > xgen-gmh					\
        && $$glib_genmarshal --prefix=gtk_marshal gtkmarshal.list --header >> xgen-gmh		\
	&& echo "#endif /* GTK_DISABLE_DEPRECATED */" >> xgen-gmh				\
	&& (cmp -s xgen-gmh gtkmarshal.h || cp xgen-gmh gtkmarshal.h)				\
	&& rm -f xgen-gmh 									\
	&& echo timestamp > $(@F)
$(srcdir)/gtkmarshal.c: @REBUILD@ gtkmarshal.list
	case @GLIB_GENMARSHAL@ in								\
	   .*) glib_genmarshal=`pwd`/@GLIB_GENMARSHAL@ ;;					\
	   *) glib_genmarshal=@GLIB_GENMARSHAL@ ;;						\
	esac;											\
	cd $(srcdir)										\
	&& $$glib_genmarshal --prefix=gtk_marshal gtkmarshal.list --body >> xgen-gmc		\
	&& cp xgen-gmc gtkmarshal.c								\
	&& rm -f xgen-gmc 

$(srcdir)/gtktypebuiltins.h: stamp-gtktypebuiltins.h
	@true
stamp-gtktypebuiltins.h: @REBUILD@ $(gtk_public_h_sources) Makefile
	( cd $(srcdir) && glib-mkenums \
			--fhead "#ifndef __GTK_TYPE_BUILTINS_H__\n#define __GTK_TYPE_BUILTINS_H__\n\n#include <glib-object.h>\n\nG_BEGIN_DECLS\n" \
			--fprod "/* enumerations from \"@filename@\" */\n" \
			--vhead "GType @enum_name@_get_type (void);\n#define GTK_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" \
			--ftail "G_END_DECLS\n\n#endif /* __GTK_TYPE_BUILTINS_H__ */" \
		$(gtk_public_h_sources) ) >> xgen-gtbh \
	&& (cmp -s xgen-gtbh $(srcdir)/gtktypebuiltins.h || cp xgen-gtbh $(srcdir)/gtktypebuiltins.h ) \
	&& rm -f xgen-gtbh \
	&& echo timestamp > $(@F)
$(srcdir)/gtktypebuiltins.c: @REBUILD@ $(gtk_public_h_sources) Makefile
	( cd $(srcdir) && glib-mkenums \
			--fhead "#undef GTK_DISABLE_DEPRECATED\n#define GTK_ENABLE_BROKEN\n#include \"gtk.h\"\n#include \"gtkprivate.h\"" \
			--fprod "\n/* enumerations from \"@filename@\" */" \
			--vhead "GType\n@enum_name@_get_type (void)\n{\n  static GType etype = 0;\n  if (etype == 0) {\n    static const G@Type@Value values[] = {" \
			--vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
			--vtail "      { 0, NULL, NULL }\n    };\n    etype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n  return etype;\n}\n" \
		$(gtk_public_h_sources) ) > xgen-gtbc \
	&& cp xgen-gtbc $(srcdir)/gtktypebuiltins.c  \
	&& rm -f xgen-gtbc

# target platform:
lib_LTLIBRARIES = $(gtktargetlib)

gtkincludedir = $(includedir)/gtk-2.0/gtk
gtkinclude_HEADERS = $(gtk_public_h_sources) $(gtk_built_public_sources) gtkversion.h

libgtk_x11_2_0_la_SOURCES = $(gtk_c_sources) $(gtk_plug_c_sources)
libgtk_linux_fb_2_0_la_SOURCES = $(gtk_c_sources)
libgtk_win32_2_0_la_SOURCES = $(gtk_c_sources)

libgtk_win32_2_0_la_LIBADD = $(gtk_win32res_lo)
libgtk_win32_2_0_la_DEPENDENCIES = $(gtk_def) $(gtk_win32res_lo)

if USE_WIN32
libgtk_target_ldflags = $(gtk_win32_symbols) -lwsock32
endif
EXTRA_LTLIBRARIES = libgtk-x11-2.0.la libgtk-linux-fb-2.0.la libgtk-win32-2.0.la

install-exec-hook: 
if DISABLE_EXPLICIT_DEPS
	$(SHELL) $(top_srcdir)/sanitize-la.sh $(DESTDIR)$(libdir)/$(gtktargetlib)
endif

# Install a RC file for the default GTK+ theme, and key themes
install-data-local: install-ms-lib install-libtool-import-lib
	$(mkinstalldirs) $(DESTDIR)$(datadir)/themes/Default/gtk-2.0
	$(INSTALL_DATA) $(srcdir)/gtkrc.default $(DESTDIR)$(datadir)/themes/Default/gtk-2.0/gtkrc
	$(mkinstalldirs) $(DESTDIR)$(datadir)/themes/Default/gtk-2.0-key
	$(INSTALL_DATA) $(srcdir)/gtkrc.key.default $(DESTDIR)$(datadir)/themes/Default/gtk-2.0-key/gtkrc
	$(mkinstalldirs) $(DESTDIR)$(datadir)/themes/Emacs/gtk-2.0-key
	$(INSTALL_DATA) $(srcdir)/gtkrc.key.emacs $(DESTDIR)$(datadir)/themes/Emacs/gtk-2.0-key/gtkrc

uninstall-local: uninstall-ms-lib uninstall-libtool-import-lib
	rm -f $(DESTDIR)$(datadir)/themes/Default/gtk-2.0/gtkrc
	rm -f $(DESTDIR)$(datadir)/themes/Default/gtk-2.0-key/gtkrc
	rm -f $(DESTDIR)$(datadir)/themes/Emacs/gtk-2.0-key/gtkrc

DEPS = $(gtktargetlib) $(top_builddir)/gdk-pixbuf/libgdk_pixbuf-$(GTK_API_VERSION).la $(top_builddir)/gdk/$(gdktargetlib)

TEST_DEPS = $(DEPS) gtk.immodules

LDADDS = @STRIP_BEGIN@ \
	$(gtktargetlib)							\
	$(top_builddir)/gdk-pixbuf/libgdk_pixbuf-$(GTK_API_VERSION).la	\
	$(top_builddir)/gdk/$(gdktargetlib) 				\
@STRIP_END@

#
# Installed tools
#
bin_PROGRAMS = gtk-query-immodules-2.0

gtk_query_immodules_2_0_DEPENDENCIES = $(DEPS)
gtk_query_immodules_2_0_LDADD = $(LDADDS)

gtk_query_immodules_2_0_SOURCES = queryimmodules.c

.PHONY: files test test-debug

files:
	@files=`ls $(DISTFILES) 2> /dev/null `; for p in $$files; do \
	  echo $$p; \
	done

EXTRA_DIST += @STRIP_BEGIN@ 	\
	line-arrow.xbm 		\
	line-wrap.xbm 		\
	tree_plus.xbm		\
	tree_minus.xbm          \
	tree_minus.xpm		\
	tree_plus.xpm		\
	gtk.def			\
	gtk-win32.rc		\
	gtk-win32.rc.in		\
	gtkrc.default		\
	gtkrc.key.default	\
	gtkrc.key.emacs		\
	makefile.msc		\
	makefile.msc.in		\
@STRIP_END@

install-data-local:

