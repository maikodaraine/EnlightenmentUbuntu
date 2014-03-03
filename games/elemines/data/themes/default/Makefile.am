MAINTAINERCLEANFILES = Makefile.in
EXTRA_DIST = default.edc default-big.edc

SUBDIRS = edc fonts images sounds

EDJE_CC = @edje_cc@
EDJE_FLAGS = \
	     -id $(top_srcdir)/data/themes/default/images \
	     -fd $(top_srcdir)/data/themes/default/fonts \
	     -sd $(top_srcdir)/data/themes/default/sounds

filesdir = $(pkgdatadir)/themes
files_DATA = default.edj default-big.edj

default.edj: Makefile default.edc
	$(EDJE_CC) $(EDJE_FLAGS) \
	$(top_srcdir)/data/themes/default/default.edc \
	$(top_builddir)/data/themes/default/default.edj

default-big.edj: Makefile default-big.edc
	$(EDJE_CC) $(EDJE_FLAGS) \
	$(top_srcdir)/data/themes/default/default-big.edc \
	$(top_builddir)/data/themes/default/default-big.edj

clean-local:
	rm -f *.edj
	rm -rf autom4te.cache

