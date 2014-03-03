MAINTAINERCLEANFILES = Makefile.in

AM_CPPFLAGS = -I. \
	   -I$(top_srcdir) \
	   -I$(includedir) \
	   @E_CFLAGS@

pkgdir = $(datadir)/$(MODULE_ARCH)
pkg_LTLIBRARIES = module.la
module_la_SOURCES = e_mod_main.h \
	e_mod_main.c \
	elev8.cc \
	elev8.h \
	timer.cc \
	timer.h \
	storage.cc \
	storage.h \
	environment.cc \
	environment.h \
	utils.cc \
	utils.h \
	gadcon.cc \
	gadcon.h

module_la_LIBADD = @E_LIBS@ -ldl -lv8 -lstdc++
module_la_LDFLAGS = -module -avoid-version
module_la_DEPENDENCIES = $(top_builddir)/config.h

clean-local:
	rm -rf *~
