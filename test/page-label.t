#!/usr/bin/env perl
use warnings;
use strict;
use utf8;
use 5.010;

use Test::Pdfout::Command tests => 3;
use Test::More;
use Testlib;

set_get_test(
    command => ['pagelabels'],
    
    broken_input => [
	'"abc"',
	],
    input => <<'EOD'
[
  {
    "page": 1
  }
]
EOD
); 
