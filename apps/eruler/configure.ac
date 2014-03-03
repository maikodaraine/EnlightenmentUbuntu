dnl Process this file with autoconf to produce a configure script.

AC_INIT([eruler], [0.1.0], [enlightenment-devel@lists.sourceforge.net])
AC_PREREQ([2.60])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.6 dist-bzip2 foreign subdir-objects])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_USE_SYSTEM_EXTENSIONS
AC_PROG_CC
AM_PROG_CC_C_O
AC_C___ATTRIBUTE__

efl_version="1.7.0"
AC_SUBST(efl_version)

requirements="\
   elementary >= ${efl_version} \
   eina >= ${efl_version} \
   evas >= ${efl_version} \
   ecore >= ${efl_version} \
   ecore-file >= ${efl_version} \
   ecore-input >= ${efl_version} \
   edje >= ${efl_version} \
   "

want_ecore_x="yes"
AC_ARG_ENABLE([ecore-x],
   [AC_HELP_STRING([--disable-ecore-x], [disable ecore-x support. @<:@default=enabled@:>@])],
   [want_ecore_x=$enableval], [])
if test "$want_ecore_x" != "no"; then
   requirements="${requirements} ecore-x >= ${efl_version}"
fi

PKG_CHECK_MODULES([ERULER], [${requirements}])

if test "$want_ecore_x" != "no"; then
   AC_DEFINE([HAVE_ECORE_X], [1], [Should use Ecore_X])
fi
AM_CONDITIONAL([HAVE_ECORE_X], [test "$want_ecore_x" != "no"])

EFL_WITH_BIN([edje], [edje-cc], [edje_cc])

with_max_log_level=""
AC_ARG_WITH(maximum-log-level,
   [AC_HELP_STRING([--with-maximum-log-level=NUMBER],
                   [limit eruler log level to the given number, any call to EINA_LOG() with values greater than this will be compiled out, ignoring runtime settings, but saving function calls.])],
   [
    if test "x${withval}" != "xno"; then
       if echo "${withval}" | grep -E '^[[0-9]]+$' >/dev/null 2>/dev/null; then
          AC_MSG_NOTICE([ignoring any EINA_LOG() with level greater than ${withval}])
          with_max_log_level="${withval}"
       else
          AC_MSG_ERROR([--with-maximum-log-level takes a decimal number, got "${withval}" instead.])
       fi
    fi
    ], [:])

if test -n "${with_max_log_level}"; then
   AC_DEFINE_UNQUOTED(EINA_LOG_LEVEL_MAXIMUM, ${with_max_log_level}, [if set, logging is limited to this amount.])
fi

AC_CONFIG_FILES([
Makefile
])

AC_OUTPUT

#####################################################################
## Info

echo
echo
echo
echo "------------------------------------------------------------------------"
echo "$PACKAGE $VERSION"
echo "------------------------------------------------------------------------"
echo
echo "Compilation................: make (or gmake)"
echo "  CPPFLAGS.................: $CPPFLAGS"
echo "  CFLAGS...................: $CFLAGS"
echo "  LDFLAGS..................: $LDFLAGS"
echo
echo "Installation...............: make install (as root if needed, with 'su' or 'sudo')"
echo "  prefix...................: $prefix"
echo
echo "Platforms:"
echo "  X11......................: ${want_ecore_x}"
