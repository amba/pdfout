#!/usr/bin/env perl
use warnings;
use strict;
use 5.020;

use Test::Pdfout::Command tests => 5;
use Testlib;
use File::Copy qw/cp/;

sub new_broken_pdf {
    my $pdf = new_tempfile();
    cp( test_data("broken.pdf"), $pdf )
        or die "cp";

    return $pdf;
}

my $pdf = new_broken_pdf();

pdfout_ok( command => [ 'repair', '--check', $pdf ], status => 1 );

pdfout_ok( command => [ 'repair', $pdf ] );

pdfout_ok( command => [ 'repair', '--check', $pdf ] );

# output option

$pdf = new_broken_pdf();
my $output = new_tempfile();
pdfout_ok( command => [ 'repair', $pdf, '-o', $output ] );
pdfout_ok( command => [ 'repair', '--check', $output ] );

