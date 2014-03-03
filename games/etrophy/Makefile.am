ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src data doc

if HAVE_PO
SUBDIRS += po
endif

MAINTAINERCLEANFILES = \
Makefile.in \
config.cache-env \
aclocal.m4 \
compile \
config.guess \
config.h.in \
config.h.in~ \
config.sub \
configure \
depcomp \
install-sh \
ltmain.sh \
missing \
$(PACKAGE_TARNAME)-$(PACKAGE_VERSION).tar.gz \
$(PACKAGE_TARNAME)-$(PACKAGE_VERSION).tar.bz2 \
$(PACKAGE_TARNAME)-$(PACKAGE_VERSION)-doc.tar.bz2 \
m4/libtool.m4 \
m4/lt~obsolete.m4 \
m4/ltoptions.m4 \
m4/ltsugar.m4 \
m4/ltversion.m4 \
po/boldquot.sed \
po/en@boldquot.header \
po/remove-potcdate.sed \
po/en@quot.header \
po/insert-header.sin \
po/Makefile.in.in* \
po/Makevars.template \
po/quot.sed \
po/remove-potcdate.sin \
po/Rules-quot* \
ABOUT-NLS \
INSTALL \
config.rpath         \
m4/codeset.m4        \
m4/fcntl-o.m4        \
m4/gettext.m4        \
m4/glibc2.m4         \
m4/glibc21.m4        \
m4/iconv.m4          \
m4/intdiv0.m4        \
m4/intl.m4           \
m4/intldir.m4        \
m4/intlmacosx.m4     \
m4/intmax.m4         \
m4/inttypes-pri.m4   \
m4/inttypes_h.m4     \
m4/lcmessage.m4      \
m4/lib-ld.m4         \
m4/lib-link.m4       \
m4/lib-prefix.m4     \
m4/lock.m4           \
m4/longlong.m4       \
m4/nls.m4            \
m4/po.m4             \
m4/printf-posix.m4   \
m4/progtest.m4       \
m4/size_max.m4       \
m4/stdint_h.m4       \
m4/threadlib.m4      \
m4/uintmax_t.m4      \
m4/visibility.m4     \
m4/wchar_t.m4        \
m4/wint_t.m4         \
m4/xsize.m4

bin_SCRIPTS =

EXTRA_DIST = \
AUTHORS \
COPYING \
autogen.sh \
etrophy.pc.in \
etrophy.spec \
README

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = etrophy.pc

.PHONY: doc

# Documentation

doc:
	@echo "entering doc/"
	make -C doc doc
