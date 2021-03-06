=-------##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
m4_define([v_maj], [0])
m4_define([v_min], [1])
m4_define([v_mic], [0])
m4_define([v_rev], m4_esyscmd([(git rev-list --count HEAD 2>/dev/null || echo 0) | tr -d '\n']))dnl
##--   When released, remove the dnl on the below line
dnl m4_undefine([v_rev])
##--   When doing snapshots - change soname. remove dnl on below line
m4_define([relname], [ver-pre-svn-09])
m4_define([v_rel], [-release relname])
##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
m4_ifdef([v_rev], [m4_define([v_ver], [v_maj.v_min.v_mic.v_rev])],
[m4_define([v_ver], [v_maj.v_min.v_mic])])
m4_define([lt_rev], m4_eval(v_maj + v_min))
m4_define([lt_cur], v_mic)
m4_define([lt_age], v_min)
##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##

AC_INIT([elev8], [v_ver], [enlightenment-devel@lists.sourceforge.net])
AC_PREREQ(2.52)
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_BUILD
AC_CANONICAL_HOST

AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.6 dist-bzip2])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_GNU_SOURCE

AC_C_BIGENDIAN
AC_ISC_POSIX
AC_PROG_CXX
AC_PROG_CC
AC_HEADER_STDC
AC_C_CONST
AC_C___ATTRIBUTE__
AC_FUNC_ALLOCA

EFL_COMPILER_FLAG([-Wall])
EFL_COMPILER_FLAG([-Wextra])
EFL_COMPILER_FLAG([-Wundef])
EFL_COMPILER_FLAG([-Wcast-align])
EFL_COMPILER_FLAG([-Wformat=2])
EFL_COMPILER_FLAG([-Wmissing-include-dirs])
EFL_COMPILER_FLAG([-Wmissing-format-attribute])
EFL_COMPILER_FLAG([-Wuninitialized])
EFL_COMPILER_FLAG([-Winit-self])
EFL_COMPILER_FLAG([-Wfloat-equal])
EFL_COMPILER_FLAG([-Wmissing-declarations])
EFL_COMPILER_FLAG([-Wshadow])
EFL_COMPILER_FLAG([-Wendif-labels])
EFL_COMPILER_FLAG([-Wstrict-aliasing])
EFL_COMPILER_FLAG([-Wno-long-long])
EFL_COMPILER_FLAG([-Wno-overlength-strings])
EFL_COMPILER_FLAG([-Wno-missing-field-initializers])
EFL_COMPILER_FLAG([-Wno-format-y2k])
EFL_COMPILER_FLAG([-Wno-parentheses])
EFL_COMPILER_FLAG([-Wpointer-arith])
EFL_COMPILER_FLAG([-Wreturn-type])

AC_LIBTOOL_WIN32_DLL
define([AC_LIBTOOL_LANG_CXX_CONFIG], [:])dnl
define([AC_LIBTOOL_LANG_F77_CONFIG], [:])dnl
AC_PROG_LIBTOOL

##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
m4_ifdef([v_rev], , [m4_define([v_rev], [0])])
m4_ifdef([v_rel], , [m4_define([v_rel], [])])
AC_DEFINE_UNQUOTED(VMAJ, [v_maj], [Major version])
AC_DEFINE_UNQUOTED(VMIN, [v_min], [Minor version])
AC_DEFINE_UNQUOTED(VMIC, [v_mic], [Micro version])
AC_DEFINE_UNQUOTED(VREV, [v_rev], [Revison])
version_info="lt_rev:lt_cur:lt_age"
release_info="v_rel"
AC_SUBST(version_info)
AC_SUBST(release_info)
##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
VMAJ=v_maj
VMIN=v_min
AC_SUBST(VMAJ)
AC_SUBST(VMIN)

case "$host_os" in
  mingw32ce* | cegcc*)
    MODULE_ARCH="$host_os-$host_cpu"
    ;;
  *)
    MODULE_ARCH="$host_os-$host_cpu-v_maj.v_min.v_mic"
    ;;
esac
AC_SUBST(MODULE_ARCH)
AC_DEFINE_UNQUOTED(MODULE_ARCH, "$MODULE_ARCH", "Module architecture")

PKG_PROG_PKG_CONFIG

PKG_CHECK_MODULES([EINA], [eina >= 1.0.999])
PKG_CHECK_MODULES([ECORE], [ecore >= 1.0.999])
PKG_CHECK_MODULES([EET], [eet >= 1.4.0])
PKG_CHECK_MODULES([ELEMENTARY], [
	elementary >= 0.7.0
	edje >= 1.0.999
	eet >= 1.4.0
	evas >= 1.0.999
	ecore-evas >= 1.0.999
])
PKG_CHECK_MODULES([ECORECON], [ecore-con >= 1.0.999])
PKG_CHECK_MODULES([ELDBUS], [eldbus])
PKG_CHECK_MODULES([EIO], [eio])

# Detect systemd user session directory properly
EFL_PKG_CHECK_VAR([USER_SESSION_DIR], [systemd], [systemduserunitdir],
	[have_systemd_user_session="yes"], [have_systemd_user_session="no"])
AM_CONDITIONAL([HAVE_SYSTEMD_USER_SESSION], [test "x${have_systemd_user_session}" = "xyes"])
AC_SUBST([USER_SESSION_DIR])

dnl Check for libv8 and v8.h by linking a bit of code using v8
AC_LANG_PUSH(C++)
AC_SUBST(V8_LIBS)
AC_MSG_CHECKING([for libv8])
saved_LIBS="$LIBS"
LIBS="$LIBS -lv8"
AC_LINK_IFELSE(
[AC_LANG_PROGRAM(
[[
  #include <v8.h>
  v8::Persistent<v8::Number> n;
]],
[[
  n = v8::Persistent<v8::Number>::New(v8::Number::New(0));
  v8::Handle<v8::Object> o = v8::Object::New();
  o->HasOwnProperty(v8::String::New("bazinga"));
]],
)],[V8_LIBS="-lv8"], AC_MSG_ERROR([elev8 requires libv8 >= 3.5.0]))
LIBS="$saved_LIBS"
AC_MSG_RESULT([yes])
AC_LANG_POP

dnl Look for libdl
AC_SEARCH_LIBS([dlopen], [dl dld], [], [
  AC_MSG_ERROR([unable to find the dlopen() function])
])

EFL_WITH_BIN([edje], [edje-cc], [edje_cc])
EFL_WITH_BIN([eet], [eet], [eet])

AC_OUTPUT([
Makefile
src/Makefile
src/bin/Makefile
src/modules/Makefile
src/modules/http/Makefile
src/modules/elm/Makefile
src/modules/fs/Makefile
src/modules/dbus/Makefile
data/Makefile
data/images/Makefile
data/desktop/Makefile
data/javascript/Makefile
data/units/Makefile
])

#####################################################################
## Info

##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
m4_ifdef([v_rev], , [m4_define([v_rev], [0])])
m4_ifdef([v_rel], , [m4_define([v_rel], [])])
AC_DEFINE_UNQUOTED(VMAJ, [v_maj], [Major version])
AC_DEFINE_UNQUOTED(VMIN, [v_min], [Minor version])
AC_DEFINE_UNQUOTED(VMIC, [v_mic], [Micro version])
AC_DEFINE_UNQUOTED(VREV, [v_rev], [Revison])
version_info="lt_cur:lt_rev:lt_age"
release_info="v_rel"
AC_SUBST(version_info)
AC_SUBST(release_info)
##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
VMAJ=v_maj
AC_SUBST(VMAJ)

### Needed information

AC_CANONICAL_BUILD
AC_CANONICAL_HOST

case "$host_os" in
   mingw32ce*)
      MODULE_ARCH="$host_os-$host_cpu"
      ;;
   *)
      MODULE_ARCH="$host_os-$host_cpu-v_maj.v_min.v_mic"
      ;;
esac
AC_SUBST(MODULE_ARCH)
AC_DEFINE_UNQUOTED(MODULE_ARCH, "$MODULE_ARCH", "Module architecture")

echo
echo
echo
echo "------------------------------------------------------------------------"
echo "$PACKAGE $VERSION"
echo "------------------------------------------------------------------------"
echo
echo "Configuration Options Summary:"
echo
echo "  eet......................: ${eet}"
echo "  edje_cc..................: ${edje_cc}"
echo
echo "Compilation................: make (or gmake)"
echo "  CPPFLAGS.................: $CPPFLAGS"
echo "  CXXFLAGS.................: $CXXFLAGS"
echo "  CFLAGS...................: $CFLAGS"
echo "  LDFLAGS..................: $LDFLAGS"
echo
echo "Installation...............: make install (as root if needed, with 'su' or 'sudo')"
echo "  prefix...................: $prefix"
echo
