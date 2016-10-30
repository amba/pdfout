#!/usr/bin/env perl
use warnings;
use strict;
use 5.020;

use Test::Pdfout::Command;
use Test::More;
use Testlib;

my $pdf = new_pdf();

pdfout_ok(
    command      => [ 'pagecount', $pdf ],
    expected_out => "10\n"
);

test_usage_help('pagecount');

done_testing();
