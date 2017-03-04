#!/usr/bin/env perl
use warnings;
use strict;
use 5.020;

use Test::Pdfout::Command;
use Testlib;
use Test::More;

my @tests = qw/
    --incremental-update
    --incremental-update-xref
    --string-conversions
    --json
    --data
    --strsep
    /;

for my $test (@tests) {
    pdfout_ok( command => [ 'debug', $test ] );
}

test_usage_help('debug');

done_testing();
