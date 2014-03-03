AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	$(EFL_CFLAGS) \
	$(SIGC_CFLAGS)

installed_libeoxxdir = $(includedir)/eoxx-@VMAJ@
dist_installed_libeoxxdir = \
	Eo.hh     \
	Eo.xx

lib_LTLIBRARIES = \
	libeoxx.la

libeoxx_la_SOURCES = \
	Eo.cc

libeoxx_la_LIBADD = \
	$(EFL_LIBS) \
	$(SIGC_LIBS)

