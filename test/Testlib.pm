package Testlib;

use 5.020;
use warnings;
use strict;

use experimental 'signatures';
use experimental 'postderef';

use File::Temp qw/tempfile tempdir/;
use File::Spec::Functions;
use File::Copy;
use File::Basename;

use Exporter 'import';

use Test::Pdfout::Command;

our @EXPORT = qw/
new_pdf
new_tempfile
set_get_test
test_data
/;

my $tempdir = tempdir(# CLEANUP => 1
    );

sub gencommand ($command_ref, $prefix) {
    my @command = $command_ref->@*;
    $command[0] = $prefix . $command[0];
    return @command;
}
    
sub set_command ($command) {
    return gencommand($command, 'set');
}

sub get_command ($command) {
    return gencommand($command, 'get');
}

sub set_get_test (%args) {
    my $input = $args{input};
    my $expected = $args{expected};
    if (not defined $expected) {
	$expected = $input;
    }
    my $command = $args{command};

    my $pdf = new_pdf();
    
    # Set
    pdfout_ok(
	command => [set_command($command), $pdf],
	input => $input,
	);

    # Get
    pdfout_ok(
	command => [get_command($command), $pdf],
	expected_out => $expected
	);

    # Broken input
    my $broken_input = $args{broken_input};
    if ($broken_input) {
	for my $input ($broken_input->@*) {
	    my $pdf = new_pdf();
	    pdfout_ok (
		command => [set_command($command), $pdf],
		input => $input,
		status => 1
		);
	}
    }
}

sub test_data ($file) {
    my $file_dir = dirname (__FILE__);
    return catfile ($file_dir, 'data', $file);
}

	
sub new_pdf () {
    my $file = test_data ('empty10.pdf');
    my (undef, $tempfile) = tempfile(DIR => $tempdir);

    copy ($file, $tempfile)
	or die "copy";
    return $tempfile;
}

sub new_tempfile () {
    my (undef, $tempfile) = tempfile(DIR => $tempdir);
    return $tempfile;
}
