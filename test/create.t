#!/usr/bin/env perl
use warnings;
use strict;
use 5.020;

use Test::Pdfout::Command tests => 22;
use Testlib;

# pagecount
{
    my $file = new_tempfile();
    pdfout_ok (
	command => ['create', '-p3', '-o', $file]
	);
    file_like ($file, qr/Count 3/, "page count is 3");
}


# width and height

# default size is A4
{
    my $file = new_tempfile();
    pdfout_ok (
	command => ['create', '-o', $file]
	);
    file_like ($file, qr/0 0 595.* 841/, "default size is A4");
}

# Check other paper sizes
{
    my @checks = (
	['A0', '0 0 2383.* 3370'],
	['B1', '0 0 2004.* 2834'],
	['C2', '0 0 1298.* 1836'],
	['2A0', '0 0 3370.* 4767'],
	['ANSI A', '0 0 612 792'],
	);

    for my $check (@checks) {
	my $size = $check->[0];
	my $regex = qr/$check->[1]/;
	my $file = new_tempfile ();
	pdfout_ok (
	    command => ['create', '-o', $file, '-s', $size]
	    );
	file_like ($file, $regex, "file size is $size");
    }
}

# landscape option
{
    my $file = new_tempfile ();
    pdfout_ok (command => ['create', '-sANSI A', '-l', '-o', $file]);
    file_like ($file, qr/0 0 792 612/, "use landscape");
}

# height and width options
{
    my $file = new_tempfile ();
    pdfout_ok (command => [qw/create -w 100 -h 100.1 -o/, $file]);
    file_like ($file, qr/0 0 100 100.1/, "height and width options");
}

# invalid arguments
{
    #invalid size
    my @sizes = qw/A11 a-1/;
    for my $size (@sizes) {
	pdfout_ok (command => [qw/create -s $size/], status => 64);
    }

    # no width
    pdfout_ok (command => [qw/create -w 100/], status => 64);

    # negative height
    pdfout_ok (command => [qw/create -w 100 -h -100/], status => 64);
}
	
