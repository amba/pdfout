package Testlib;

use 5.010;
use warnings;
use strict;

use File::Temp qw/tempfile tempdir/;
use File::Spec::Functions;
use File::Copy;

use Exporter 'import';

our @EXPORT = qw/new_pdf/;

my $tempdir = tempdir(# CLEANUP => 1
    );
warn "tempdir: $tempdir";

sub new_pdf {
    my $file = catfile('data', 'empty10.pdf');
    my (undef, $tempfile) = tempfile(DIR => $tempdir);

    copy ($file, $tempfile)
	or die "copy";
    return $tempfile;
}
