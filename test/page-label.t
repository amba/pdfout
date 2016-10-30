#!/usr/bin/env perl
use warnings;
use strict;
use utf8;
use 5.020;

use Test::Pdfout::Command;
use Test::More;
use Testlib;

my $input = <<'EOD';
[
  {
    "page": 1
  },
  {
    "page": 2,
    "style": "arabic",
    "first": 1
  },
  {
    "page": 3,
    "style": "roman",
    "prefix": "∂ρ⇒∂↦⇒Φ"
  },
  {
    "page": 4,
    "style": "Roman"
  },
  {
    "page": 5,
    "style": "Letters"
  },
  {
    "page": 6,
    "style": "letters"
  }
]
EOD

set_get_test(
    command => ['pagelabels'],

    broken_input => [
	'"abc"',
	'[]',
	'[{"page": 2}]',
	'[{"page": -1}]',
	'[{"style": "arabic"}]',
	'[{"page": 1, "style": "goofy"}]',
	'[{"page": 1, "first": -1}]',
	'[{"page": 1}, {"page": 3}, {"page": 2}]',
	'[{"page": 1, "prefix": []}]'
    ],
	
    input        => $input,
    empty        => "[]\n",

);

done_testing();
