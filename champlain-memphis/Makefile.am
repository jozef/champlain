
BUILT_SOURCES = \
	champlain-memphis-marshal.h		 \
	champlain-memphis-marshal.c

CLEANFILES = $(BUILT_SOURCES)

CHAMPLAIN_MEMPHIS_MARSHAL = champlain-memphis-marshal
CHAMPLAIN_MEMPHIS_MARSHAL_LIST = champlain-memphis-marshal.list

lib_LTLIBRARIES = libchamplain-memphis-@CHAMPLAIN_API_VERSION@.la

libchamplain_memphis_headers = \
	champlain-bounding-box.h \
	champlain-map-data-source.h \
	champlain-memphis.h \
	champlain-memphis-tile-source.h

libchamplain_memphis_@CHAMPLAIN_API_VERSION@_la_SOURCES = \
	$(CHAMPLAIN_MEMPHIS_MARSHAL_LIST) 	\
	$(BUILT_SOURCES) 		\
	$(libchamplain_memphis_headers)		\
	champlain-bounding-box.c \
	champlain-local-map-data-source.c \
	champlain-memphis-marshal.c \
	champlain-memphis-tile-source.c \
	champlain-network-map-data-source.c

libchamplain_memphis_include_HEADERS = \
	champlain-bounding-box.h \
	champlain-map-data-source.h \
	champlain-memphis.h \
	champlain-memphis-tile-source.h

libchamplain_memphis_@CHAMPLAIN_API_VERSION@_la_SOURCES = \
	$(CHAMPLAIN_MEMPHIS_MARSHAL_LIST)		 \
	$(BUILT_SOURCES)

libchamplain_memphis_@CHAMPLAIN_API_VERSION@_la_LIBADD = $(MEMPHIS_LIBS) 	\
	$(top_builddir)/champlain/libchamplain-@CHAMPLAIN_API_VERSION@.la

champlain-memphis-marshal.h: $(CHAMPLAIN_GTK_MARSHAL_LIST)
	@GLIB_GENMARSHAL@ --header --prefix=champlain_memphis_marshal $< > $(CHAMPLAIN_MEMPHIS_MARSHAL).h

champlain-memphis-marshal.c: $(CHAMPLAIN_GTK_MARSHAL_LIST)
	@GLIB_GENMARSHAL@ --body --prefix=champlain_memphis_marshal $< > $(CHAMPLAIN_MEMPHIS_MARSHAL).c

libchamplain_memphis_includedir=$(includedir)/libchamplain-memphis-@CHAMPLAIN_API_VERSION@/champlain-memphis

libchamplain_memphis_@CHAMPLAIN_API_VERSION@_la_LDFLAGS= -version-info $(LIBRARY_VERSION)

AM_CPPFLAGS = $(MEMPHIS_CFLAGS) -I$(top_srcdir) -DCHAMPLAIN_MEMPHIS_COMPILATION
AM_LDFLAGS = $(MEMPHIS_LIBS) -export-symbols-regex ^champlain_memphis.*

EXTRA_DIST = $(CHAMPLAIN_MEMPHIS_MARSHAL_LIST)

