ACLOCAL_AMFLAGS = -I m4
MAINTAINERCLEANFILES = Makefile.in aclocal.m4 config.guess config.h.in \
		       config.sub configure depcomp install-sh ltmain.sh \
		       missing module.desktop config.rpath mkinstalldirs

SUBDIRS = src

# if HAVE_PO
# SUBDIRS += po
# endif

EDJE_FLAGS = -v \
	     -id $(top_srcdir)/images

filesdir = $(datadir)
files_DATA = module.desktop e-module-elev8.edj

EXTRA_DIST = module.desktop.in \
	     e_modules-elev8.spec.in \
	     e-module-elev8.edc \
	     images/module_icon.png

e-module-elev8.edj: Makefile e-module-elev8.edc images/module_icon.png

%.edj:  %.edc
	$(EDJE_CC) $(EDJE_FLAGS) $< $@

clean-local:
	 rm -rf *.edj module.desktop e_modules-elev8.spec *~

uninstall:
	 rm -rf $(DESTDIR)$(datadir)

