#!/usr/bin/env perl
use 5.020;
use warnings;
use strict;

use experimental 'signatures';

say 'checking out git submodules';

safe_system (qw/git submodule update --init/);

chdir 'mupdf'
    or die 'cannot change directory to mupdf';

# We do not need the curl submodule.
my @mupdf_submodles = qw(
thirdparty/freetype 
thirdparty/jbig2dec
thirdparty/jpeg
thirdparty/openjpeg
thirdparty/zlib
thirdparty/mujs
thirdparty/harfbuzz
);

safe_system (qw/git submodule update --init/, @mupdf_submodles);
	     
sub safe_system (@command) {
    system (@command) == 0
	or die "command @command failed";
}
