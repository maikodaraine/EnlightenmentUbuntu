MAINTAINERCLEANFILES = Makefile.in

bin_PROGRAMS = elemines

elemines_CPPFLAGS = -I. \
		    -DPACKAGE_BIN_DIR=\"$(bindir)\" \
		    -DPACKAGE_DATA_DIR=\"$(pkgdatadir)\" \
		    @ELEMINES_CFLAGS@

elemines_SOURCES = \
		   game.c \
		   gui.c \
		   init.c \
		   main.c

elemines_LDADD = @ELEMINES_LIBS@
elemines_LDFLAGS = $(LTLIBINTL)

localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@

EXTRA_DIST = elemines.h gettext.h
