ACLOCAL_AMFLAGS = -I m4

MAINTAINERCLEANFILES = \
Makefile.in \
$(PACKAGE_TARNAME)-$(PACKAGE_VERSION).tar.gz \
$(PACKAGE_TARNAME)-$(PACKAGE_VERSION).tar.bz2 \
aclocal.m4 \
config.guess \
config.h.in \
config.sub \
configure \
compile \
depcomp \
install-sh \
ltconfig \
ltmain.sh \
missing \
mkinstalldirs \
stamp-h.in \
stamp-h \
m4/libtool.m4 \
m4/lt~obsolete.m4 \
m4/ltoptions.m4 \
m4/ltsugar.m4 \
m4/ltversion.m4

bin_PROGRAMS = src/bin/eruler

src_bin_eruler_SOURCES = \
src/bin/main.c \
src/bin/private.h

if HAVE_ECORE_X
src_bin_eruler_SOURCES += src/bin/platform-x.c
endif

src_bin_eruler_LDADD = @ERULER_LIBS@
src_bin_eruler_CPPFLAGS = \
-I$(top_builddir) \
-DPACKAGE_BIN_DIR=\"$(bindir)\" \
-DPACKAGE_DATA_DIR=\"$(pkgdatadir)\" \
@ERULER_CFLAGS@

desktopdir = $(datadir)/applications
desktop_DATA = data/desktop/eruler.desktop

iconsdir = $(datadir)/icons
icons_DATA = data/icons/eruler.png

EDJE_CC = @edje_cc@
EDJE_FLAGS_VERBOSE_ =
EDJE_FLAGS_VERBOSE_0 =
EDJE_FLAGS_VERBOSE_1 = -v
EDJE_FLAGS = $(EDJE_FLAGS_VERBOSE_$(V))
AM_V_EDJ = $(am__v_EDJ_$(V))
am__v_EDJ_ = $(am__v_EDJ_$(AM_DEFAULT_VERBOSITY))
am__v_EDJ_0 = @echo "  EDJ   " $@;

themesdir = $(pkgdatadir)/themes
themes_DATA = data/themes/default.edj
themes_sources = data/themes/default.edc

data/themes/default.edj: Makefile data/themes/default.edc
	@$(MKDIR_P) $(top_builddir)/data/themes
	$(AM_V_EDJ)$(EDJE_CC) $(EDJE_FLAGS) \
	$(top_srcdir)/data/themes/default.edc \
	$(top_builddir)/data/themes/default.edj

EXTRA_DIST = README AUTHORS COPYING autogen.sh \
$(desktop_DATA) \
$(icons_DATA) \
$(themes_sources)

clean-local:
	rm -f $(themes_DATA)
