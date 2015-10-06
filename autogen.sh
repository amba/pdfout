#!/bin/sh
# based on GNU libunistring's autogen.sh

# Convenience script for regenerating all autogeneratable files that are
# omitted from the git repository. In particular, this script
# also regenerates all aclocal.m4, config.h.in, Makefile.in, configure files
# with new versions of autoconf or automake.
# Checks out the mupdf git-submodule.
#
# This script requires autoconf >= 2.69 and automake >= 1.14 in the PATH.
# It also requires either
#   - the GNULIB_TOOL environment variable pointing to the gnulib-tool script
#     in a gnulib checkout, or
#   - the git program in the PATH and an internet connection.

# Copyright (C) 2003-2012 Free Software Foundation, Inc.
# Copyright (C) 2015 Simon Reinhardt
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# Usage: ./autogen.sh [--skip-gnulib]

skip_gnulib=false
while :; do
  case "$1" in
    --skip-gnulib) skip_gnulib=true; shift;;
    *) break ;;
  esac
done

if test $skip_gnulib = false; then
  if test -z "$GNULIB_TOOL"; then
    # Check out gnulib in a subdirectory 'gnulib'.
    if test -d gnulib; then
      (cd gnulib && git pull)
    else
      git clone git://git.savannah.gnu.org/gnulib.git
    fi
    # Now it should contain a gnulib-tool.
    if test -f gnulib/gnulib-tool; then
      GNULIB_TOOL=`pwd`/gnulib/gnulib-tool
    else
      echo "** warning: gnulib-tool not found" 1>&2
    fi
  fi
  # Skip the gnulib-tool step if gnulib-tool was not found.
  if test -n "$GNULIB_TOOL"; then
    GNULIB_MODULES='
      argp
      argmatch
      clean-temp
      dirname
      error
      tempname
      xvasprintf
      xmemdup0
      regex
      regex-quote
      manywarnings
      progname
      copy-file
      full-read
      full-write
    '
    $GNULIB_TOOL --source-base=gl --m4-base=gl/m4 --tests-base=gl-tests \
		  --with-tests --import $GNULIB_MODULES
    
  fi
fi

# checkout submodules
echo "checking out git-submodules"
git submodule update --init
# we don't need the curl and mujs submodules
(cd mupdf && git submodule update --init thirdparty/freetype \
  thirdparty/jbig2dec  thirdparty/jpeg thirdparty/openjpeg thirdparty/zlib)

# generate configure and Makefile.ins
echo "running autoreconf -fiv"
autoreconf -iv
