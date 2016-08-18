#!/usr/bin/env perl
use warnings;
use strict;
use utf8;
use 5.020;

use Test::Pdfout::Command tests => 6;
use Test::Files;
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
    
    broken_input => [
	'"abc"',
	],
    input => $input
    );

# check default filename option.

{
    my $pdf = new_pdf ();
    
    my $default_filename = $pdf . '.pagelabels';
    open my $fh, '>', $default_filename
	or die "open";
    print {$fh} $input;
    close $fh
	or die "close";

    pdfout_ok (
	command => ['setpagelabels', $pdf, '-d'],
	);

    unlink ($default_filename)
	or die "unlink";

    pdfout_ok (
	command => ['getpagelabels', $pdf, '-d'],
	);

    file_ok ($default_filename, $input, "file matches input");
}
    
