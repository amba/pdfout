#!/usr/bin/env perl
use warnings;
use strict;
use utf8;
use 5.010;

use Test::Pdfout::Command tests => 5;
use Test::More;
use Testlib;

set_get_test(
    command => ['info'],
    
    broken_input => [
	# should be 'Title'
	'{"Titel": "myfile"}',
	# should be 'True'
	'{"Trapped": "true"}',
	# open not a bool
	'{"ModDate": "D:20151"}'
	],
    input => <<'EOD'
{
  "Title": "pdfout info dict test",
  "Author": "pdfout",
  "Subject": "testing",
  "Keywords": "la la la",
  "Creator": "none",
  "Producer": "la la",
  "CreationDate": 20150701,
  "ModDate": "D:20150702",
  "Trapped": "Unknown"
}
EOD
); 
