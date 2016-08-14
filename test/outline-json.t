#!/usr/bin/env perl
use warnings;
use strict;
use 5.010;

use Test::Pdfout::Command tests => 3;
use Test::More;
use Testlib;

my @broken_input = (
    # no title
    '[{"page": 1}]',
    # page number too big
    '[{"title": "abc", "page": 11}]',
    # open not a bool
    '[{"title": "abc", "page": 1, "open": "a"}]');

for my $broken (@broken_input) {
    my $file = new_pdf();
    diag("file: $file");
    pdfout_ok (
	command => ['setout', $file],
	input => $broken,
	status => 1
	);
}
