#!/usr/bin/env perl
use warnings;
use strict;
use 5.024;

use Test::Pdfout::Command tests => 5;

my @tests = qw/
--incremental-update
--incremental-update-xref
--string-conversions
--regex
--json
/;

for my $test (@tests) {
    pdfout_ok (command => ['debug', $test]);
}
