bin_PROGRAMS = \
	eoxx_test

eoxx_test_SOURCES = \
	eoxx_test.cc
eoxx_test_CPPFLAGS = \
	-I$(top_srcdir)/src/lib/ \
	-I$(top_builddir)/src/lib/ \
	-DPACKAGE_BUILD_DIR=\"`pwd`/$(top_builddir)/\" \
	@EFL_CFLAGS@ @SIGC_CFLAGS@
eoxx_test_LDADD = $(top_srcdir)/src/lib/libeoxx.la @EFL_LIBS@ @SIGC_LIBS@

