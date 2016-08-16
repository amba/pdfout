package Testlib;

use 5.010;
use warnings;
use strict;

use File::Temp qw/tempfile tempdir/;
use File::Spec::Functions;
use File::Copy;

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

sub gencommand {
    my ($command_ref, $prefix) = @_;
    my @command = @{$command_ref};
    $command[0] = $prefix . $command[0];
    return @command;
}
    
sub set_command {
    return gencommand($_[0], 'set');
}

sub get_command {
    return gencommand($_[0], 'get');
}

sub set_get_test {
    my %args = @_;
    my $input = $args{input};
    my $expected_out = $args{expected_out};
    if (not defined $expected_out) {
	$expected_out = $input;
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
	expected_out => $expected_out
	);

    # Broken input
    my $broken_input = $args{broken_input};
    if ($broken_input) {
	for my $input (@{$broken_input}) {
	    my $pdf = new_pdf();
	    pdfout_ok (
		command => [set_command($command), $pdf],
		input => $input,
		status => 1
		);
	}
    }
}

sub test_data {
    my $file = shift;
    return catfile('data', $file);
}

	
sub new_pdf {
    my $file = catfile('data', 'empty10.pdf');
    my (undef, $tempfile) = tempfile(DIR => $tempdir);

    copy ($file, $tempfile)
	or die "copy";
    return $tempfile;
}

sub new_tempfile {
    my (undef, $tempfile) = tempfile(DIR => $tempdir);
    return $tempfile;
}
