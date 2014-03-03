AC_INIT([elemines], [0.3], [ngc891@gmail.com])
AC_PREREQ([2.65])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_HEADERS([config.h])
AC_CANONICAL_BUILD
AC_CANONICAL_HOST

AM_INIT_AUTOMAKE([1.10 dist-bzip2])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# this will speed up libtool checks
LT_PREREQ([2.2])
LT_INIT([win32-dll])

AC_PROG_CC
AM_PROG_CC_C_O
EFL_ATTRIBUTE_UNUSED
ELM_QUICKLAUNCH
EFL_COMPILER_FLAG([-Wall])
EFL_COMPILER_FLAG([-W])

# Checks for header files.
EFL_CHECK_PATH_MAX

# Check for EFL
PKG_CHECK_MODULES([ELEMINES],
[
 eina >= 1.8.0
 evas >= 1.8.0
 ecore >= 1.8.0
 edje >= 1.8.0
 elementary >= 1.8.0
 etrophy >= 0.5.1
])

# Check edje_cc
EFL_WITH_BIN([edje], [edje-cc], [edje_cc])

# Checks for library functions.
AC_CHECK_FUNCS([setlocale])

AM_GNU_GETTEXT_VERSION([0.17])
AM_GNU_GETTEXT([external])

AC_CONFIG_FILES([
Makefile
data/Makefile
data/desktop/Makefile
data/themes/Makefile
data/themes/default/Makefile
data/themes/default/edc/Makefile
data/themes/default/fonts/Makefile
data/themes/default/images/Makefile
data/themes/default/sounds/Makefile
doc/Makefile
packaging/Makefile
packaging/slackware/Makefile
po/Makefile.in
src/Makefile
])
AC_OUTPUT

echo
echo
echo "------------------------------------------------------------------------"
echo "$PACKAGE_NAME $PACKAGE_VERSION"
echo "------------------------------------------------------------------------"
echo
echo "Compilation..............: make (or gmake)"
echo "  CFLAGS.................: $CFLAGS"
echo "  edje_cc................: ${edje_cc}"
echo
echo "Installation.............: make install (as root if needed, with 'su' or 'sudo')"
echo "  prefix.................: $prefix"
echo
