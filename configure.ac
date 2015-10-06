#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([pdfout], [0.1], [github.com/amba/pdfout/issues])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([1.11 -Wall -Werror foreign])
AM_MAINTAINER_MODE
AC_CONFIG_MACRO_DIRS([m4 gl/m4])
AC_CONFIG_SRCDIR([src/outline.c])
AC_CONFIG_HEADERS([config.h])
dnl Checks for programs.

AC_PROG_CC_C99
gl_EARLY

m4_ifdef([AM_PROG_AR], [AM_PROG_AR]) dnl workaround for automake 1.11

AC_PROG_INSTALL
AC_PROG_RANLIB

gl_MANYWARN_ALL_GCC([warnings])
# Set up the list of the pointless, undesired warnings.
nw="-Wdouble-promotion -Wshadow -Wvariadic-macros"
nw="$nw -Wsuggest-attribute=noreturn -Wjump-misses-init"
# Enable all GCC warnings not in this list.
gl_MANYWARN_COMPLEMENT([warnings], [$warnings], [$nw])

warnings="$warnings -Wno-error=unused-parameter"
warnings="$warnings -Wno-missing-field-initializers"
warnings="$warnings -Wno-error=clobbered"


AC_ARG_ENABLE([warnings],
	[AS_HELP_STRING([--enable-warnings],
	[enable many compiler warnings])],
	[AS_IF([test "X$enableval" = "Xyes"],
	       [for w in $warnings;
	          do gl_WARN_ADD([$w], [WARN_CFLAGS])
		done
	       ])])
	       

AC_ARG_ENABLE([Werror],
	[AS_HELP_STRING([--enable-Werror],
			[compile with -Werror])],
	[AS_IF([test "X$enableval" = "Xyes"], 
               [gl_WARN_ADD([-Werror], [WARN_CFLAGS])])])
	      
dnl Checks for libraries.
dnl use pkg-config if possible

PKG_CHECK_MODULES([LIBYAML], [yaml-0.1 >= 0.1.4])
PKG_CHECK_MODULES([LIBCRYPTO], [libcrypto >= 1.0.1e])

AC_SEARCH_LIBS([u8_conv_from_encoding], [unistring], [],
               [AC_MSG_FAILURE([missing library])])
AC_CHECK_HEADERS([unistr.h uniconv.h],
                 [], [AC_MSG_FAILURE([missing headers])])

dnl build mupdf submodule
dnl is it checked out?
AS_IF([test -r ${srcdir}/mupdf/README], [],
  [AC_MSG_ERROR([mupdf submodule is not checked out.
Try 'git submodule update --init'])])
third=${srcdir}/mupdf/thirdparty
AS_IF([test -r ${third}/openjpeg/libopenjpeg], [],
      [AC_MSG_ERROR([mupdf's submodules are not checked out.
Try './git-submodule-update.sh'])])
  
AX_CHECK_GNU_MAKE()
AS_IF([test "X${_cv_gnu_make_command}" != "X"],
[AC_SUBST([GNU_MAKE], ["${_cv_gnu_make_command}"])],
  [AC_MSG_ERROR([need GNU make to build mupdf])])

AC_ARG_ENABLE([mupdf-build],
  [AS_HELP_STRING([--enable-mupdf-build=BUILD],
  [use build=BUILD on the make command line when compiling mupdf.
  Must be one of release(default),debug,native,coverage,profile,memento])],
  [mupdf_build=${enableval}], [mupdf_build=release])
  
AC_SUBST([mupdf_build])

gl_INIT

AC_SUBST([MUPDF_REQUIRED_LIBS], ["-lm $LIBYAML_LIBS $LIBCRYPTO_LIBS $LIBS"])
LIBS=

AC_CONFIG_FILES([Makefile gl/Makefile gl-tests/Makefile src/Makefile
	         src/test/Makefile src/program/Makefile])

AC_OUTPUT
