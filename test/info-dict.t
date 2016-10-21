#!/usr/bin/env perl
use warnings;
use strict;
use utf8;
use 5.020;

use Test::Pdfout::Command tests => 18;
use Test::More;
use Test::Files;
use Testlib;

my $input = <<'EOD';
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
    
set_get_test (
    command => ['info'],
    
    broken_input => [
	# should be 'Title'
	'{"Titel": "myfile"}',
	# should be 'True'
	'{"Trapped": "true"}',
	# open not a bool
	'{"ModDate": "D:20151"}'
    ],
    input => $input,
    empty => "{}\n",
    ); 

# append option
{
    my $pdf = new_pdf ();
    pdfout_ok (
	command => ['setinfo', $pdf],
	input => $input,
	);
    
    pdfout_ok (
	command => ['setinfo', '--append', $pdf],
	input => '{"CreationDate": "D:20150804"}',
	);

    pdfout_ok (
	command => ['getinfo', $pdf],
	expected_out => <<'EOD',
{
  "Title": "pdfout info dict test",
  "Author": "pdfout",
  "Subject": "testing",
  "Keywords": "la la la",
  "Creator": "none",
  "Producer": "la la",
  "CreationDate": "D:20150804",
  "ModDate": "D:20150702",
  "Trapped": "Unknown"
}
EOD
	);
}
