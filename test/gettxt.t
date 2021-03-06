#!/usr/bin/env perl
use warnings;
use strict;
use 5.020;

use Test::Pdfout::Command;
use Test::More;

use Testlib;
use File::Copy qw/cp/;

my $pdf = new_tempfile();
cp( test_data("hello-world.pdf"), $pdf )
    or die "cp";

my $expected = <<"EOD";
Hello, World from page 1!
\f
Hello, World from page 2!
\f
Hello, World from page 3!
\f
EOD

pdfout_ok( command => [ 'gettxt', $pdf ], expected_out => $expected );

my $expected_page_1 = "Hello, World from page 1!\n\f\n";

pdfout_ok(
    command      => [ 'gettxt', '-p1', $pdf ],
    expected_out => $expected_page_1
);

pdfout_ok(
    command      => [ 'gettxt', '-p1, 2-3', $pdf ],
    expected_out => $expected
);

test_usage_help('gettxt');

done_testing();
# FIXME: test page range parser in whitebox tests?
