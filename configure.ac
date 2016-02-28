#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([pdfout], [0.1], [github.com/amba/pdfout/issues])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([1.11 -Wall -Werror foreign])
AM_SILENT_RULES([yes])
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
nw="-W -Wextra -Wdouble-promotion -Wshadow -Wvariadic-macros \
-Wsuggest-attribute=noreturn -Wjump-misses-init -Wmissing-field-initializers"
# Enable all GCC warnings not in this list.
gl_MANYWARN_COMPLEMENT([warnings], [$warnings], [$nw])


AC_ARG_ENABLE([warnings],
	[AS_HELP_STRING([--enable-warnings],
	[enable many compiler warnings])],
	[AS_IF([test "$enableval" = "yes"],
	       [for w in $warnings;
	          do gl_WARN_ADD([$w], [WARN_CFLAGS])
		done
	       ])])
	       

AC_ARG_ENABLE([Werror],
	[AS_HELP_STRING([--enable-Werror],
			[compile with -Werror])],
	[AS_IF([test "$enableval" = "yes"], 
               [gl_WARN_ADD([-Werror], [WARN_CFLAGS])
	        gl_WARN_ADD([-Wno-error=suggest-attribute=pure], [WARN_CFLAGS])
		gl_WARN_ADD([-Wno-error=unused-parameter], [WARN_CFLAGS])
		gl_WARN_ADD([-Wno-error=suggest-attribute=format],)
		gl_WARN_ADD([-Wno-error=inline],		
		            [WARN_CFLAGS])
		gl_WARN_ADD([-Wno-error=clobbered], [WARN_CFLAGS])])])
	      
dnl Checks for libraries.
dnl use pkg-config if possible

PKG_CHECK_MODULES([LIBYAML], [yaml-0.1 >= 0.1.4])

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
AS_IF([test -n "${_cv_gnu_make_command}"],
      [AC_SUBST([GNU_MAKE], ["${_cv_gnu_make_command}"])],
      [AC_MSG_ERROR([GNU make is required to build mupdf])])

dnl Set up mupdf's build command
MUPDF_MAKE_COMMAND="SYS_OPENSSL_CFLAGS= SYS_OPENSSL_LIBS="
AS_IF([test "$AM_DEFAULT_VERBOSITY" = "1"],
      [MUPDF_MAKE_COMMAND=="$MUPDF_MAKE_COMMAND verbose=yes"])

MUPDF_XCFLAGS="-g"
AC_ARG_VAR([MUPDF_CFLAGS], [C compiler flags used to build mupdf])
AS_IF([test -n "$MUPDF_CFLAGS"],
      [MUPDF_XCFLAGS="$MUPDF_XCFLAGS $MUPDF_CFLAGS],
      [MUPDF_XCFLAGS="$MUPDF_XCFLAGS $CFLAGS])

AC_SUBST([MUPDF_MAKE_COMMAND])
AC_SUBST([MUPDF_XCFLAGS])

gl_INIT

AC_SUBST([MUPDF_REQUIRED_LIBS], ["-lm $LIBYAML_LIBS $LIBS"])
LIBS=

AC_CONFIG_FILES([Makefile gl/Makefile gl-tests/Makefile src/Makefile
	         src/test/Makefile src/program/Makefile])

AC_OUTPUT
