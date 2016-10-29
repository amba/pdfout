#!/usr/bin/env perl
use warnings;
use strict;
use 5.020;

use Test::Pdfout::Command tests => 6;

pdfout_ok(
    command      => ['-h'],
    expected_out => qr/\n  -V, --version/
);

pdfout_ok(
    command      => ['-V'],
    expected_out => qr/GPL/
);

pdfout_ok(
    command      => ['-l'],
    expected_out => qr/setoutline/
);

pdfout_ok(
    command      => ['-d'],
    expected_out => qr/Dump outline/
);

# invalid command
pdfout_ok(
    command => ['uiaeuiae'],
    status  => 1,
);

# missing command
pdfout_ok(
    command => [],
    status  => 1,
);

