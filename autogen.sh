#!/bin/sh

# checkout submodules
echo "checking out git-submodules"
git submodule update --init
# we don't need the curl submodule
(cd mupdf && git submodule update --init thirdparty/freetype \
		 thirdparty/jbig2dec  thirdparty/jpeg thirdparty/openjpeg \
		 thirdparty/zlib thirdparty/mujs thirdparty/harfbuzz
		 
)

# generate configure and Makefile.ins
echo "running autoreconf -fiv"
autoreconf -iv
