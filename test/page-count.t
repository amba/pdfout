#!/usr/bin/env perl
use warnings;
use strict;
use 5.024;

use Test::Pdfout::Command tests => 1;
use Testlib;

my $pdf = new_pdf ();

pdfout_ok (
    command => ['pagecount', $pdf],
    expected_out => "10\n"
    );
