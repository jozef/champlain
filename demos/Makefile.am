noinst_PROGRAMS = minimal launcher animated-marker polygons url-marker

INCLUDES = -I$(top_srcdir)

AM_CPPFLAGS = $(DEPS_CFLAGS) $(WARN_CFLAGS)
AM_LDFLAGS = $(DEPS_LIBS)

minimal_SOURCES = minimal.c
minimal_LDADD = $(DEPS_LIBS) ../champlain/libchamplain-@CHAMPLAIN_API_VERSION@.la

launcher_SOURCES = launcher.c markers.c
launcher_LDADD = $(DEPS_LIBS) ../champlain/libchamplain-@CHAMPLAIN_API_VERSION@.la

animated_marker_SOURCES = animated-marker.c
animated_marker_LDADD = $(DEPS_LIBS) ../champlain/libchamplain-@CHAMPLAIN_API_VERSION@.la

polygons_SOURCES = polygons.c
polygons_LDADD = $(DEPS_LIBS) ../champlain/libchamplain-@CHAMPLAIN_API_VERSION@.la

url_marker_SOURCES = url-marker.c
url_marker_CPPFLAGS = $(DEPS_CFLAGS) $(SOUP_CFLAGS)
url_marker_LDADD = $(SOUP_LIBS) $(DEPS_LIBS) ../champlain/libchamplain-@CHAMPLAIN_API_VERSION@.la

if ENABLE_GTK
noinst_PROGRAMS += minimal-gtk
minimal_gtk_SOURCES = minimal-gtk.c
minimal_gtk_CPPFLAGS = $(GTK_CFLAGS)
minimal_gtk_LDADD = $(GTK_LIBS) $(DEPS_LIBS) \
		../champlain-gtk/libchamplain-gtk-@CHAMPLAIN_API_VERSION@.la \
		../champlain/libchamplain-@CHAMPLAIN_API_VERSION@.la

noinst_PROGRAMS += launcher-gtk
launcher_gtk_SOURCES = launcher-gtk.c markers.c
launcher_gtk_CPPFLAGS = $(GTK_CFLAGS)
launcher_gtk_LDADD = $(GTK_LIBS) $(DEPS_LIBS) \
		../champlain-gtk/libchamplain-gtk-@CHAMPLAIN_API_VERSION@.la \
		../champlain/libchamplain-@CHAMPLAIN_API_VERSION@.la

if ENABLE_MEMPHIS
noinst_PROGRAMS += local-rendering
local_rendering_SOURCES = local-rendering.c
local_rendering_CPPFLAGS = $(GTK_CFLAGS) $(MEMPHIS_CFLAGS)
local_rendering_LDADD = $(GTK_LIBS) $(MEMPHIS_LIBS) $(DEPS_LIBS) \
		../champlain-gtk/libchamplain-gtk-@CHAMPLAIN_API_VERSION@.la \
		../champlain/libchamplain-@CHAMPLAIN_API_VERSION@.la
endif
endif

EXTRA_DIST = markers.h launcher.js

