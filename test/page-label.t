#!/usr/bin/env perl
use warnings;
use strict;
use utf8;
use 5.020;

use Test::Pdfout::Command tests => 13;
use Testlib;

my $input = <<'EOD';
[
  {
    "page": 1
  }
]
EOD

set_get_test(
    command => ['pagelabels'],

    broken_input => [ '"abc"', ],
    input        => $input,
    empty        => "[]\n",

);
