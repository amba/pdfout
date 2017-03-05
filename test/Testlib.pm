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
use Carp;
use Encode 'encode';
our @EXPORT = qw/
    new_pdf
    new_tempfile
    set_get_test
    test_data
    test_usage_help
    /;

my $tempdir = tempdir(    # CLEANUP => 1
);

sub gencommand ( $command_ref, $prefix ) {
    my @command = $command_ref->@*;
    $command[0] = $prefix . $command[0];
    return @command;
}

sub set_command ($command) {
    return gencommand( $command, 'set' );
}

sub get_command ($command) {
    return gencommand( $command, 'get' );
}

sub set_get_test (%args) {
    my $input = $args{input};
    if ( not defined $input ) {
        croak "no input";
    }

    my $expected = $args{expected};
    if ( not defined $expected ) {
        $expected = $input;
    }

    my $command = $args{command};
    if ( not defined $command ) {
        croak "no command";
    }

    my $empty = $args{empty};
    if ( not defined $empty ) {
        croak "no empty";
    }
    {
        # Set and get, using stdin/stdout
        my $pdf = new_pdf();

        pdfout_ok(
            command => [ set_command($command), $pdf ],
            input   => $input,
        );

        pdfout_ok(
            command      => [ get_command($command), $pdf ],
            expected_out => $expected
        );

        # remove
        pdfout_ok( command => [ set_command($command), '--remove', $pdf ] );
        pdfout_ok(
            command      => [ get_command($command), $pdf ],
            expected_out => $empty
        );
    }
    {
        # Set and get, using default filenames.
        my $pdf = new_pdf();

        my $default_filename = $pdf . '.' . $command->[0];
        open my $fh, '>', $default_filename
            or die "open";
        binmode $fh;
        print {$fh} $input;
        close $fh
            or die "close";

        pdfout_ok( command => [ set_command($command), $pdf, '-d' ], );

        unlink($default_filename)
            or die "unlink";

        pdfout_ok( command => [ get_command($command), $pdf, '-d' ], );
        
        file_ok($default_filename, $expected, "file matches input");
    }

    {
        # non-incremental update
        my $pdf    = new_pdf();
        my $output = new_tempfile();

        pdfout_ok(
            command => [ set_command($command), $pdf, '-o', $output ],
            input   => $input,
        );
        pdfout_ok(
            command      => [ get_command($command), $output ],
            expected_out => $expected,
        );

        compare_ok($pdf, test_data("empty10.pdf"),
                   "original file is untouched");

        # combination of remove and non-incremental
        $output = new_tempfile();
        pdfout_ok( command =>
                [ set_command($command), $pdf, '--remove', '-o', $output ], );
        pdfout_ok(
            command      => [ get_command($command), $output ],
            expected_out => $empty
        );
    }

    # Broken input
    my $broken_input = $args{broken_input};
    if ($broken_input) {
        for my $input ( $broken_input->@* ) {
            my $pdf = new_pdf();
            pdfout_ok(
                command => [ set_command($command), $pdf ],
                input   => $input,
                status  => 1
            );
        }
    }

    test_usage_help ((set_command($command))[0]);
    test_usage_help ((get_command($command))[0]);

    
}

sub test_usage_help ($command) {
    pdfout_ok(
	command => [$command, '--usage'],
	expected_out => qr/^Usage:/
	);
    pdfout_ok(
	command => [$command, '--help'],
	expected_out => qr/^Usage:.*^ Options:\n/ms
	);
}

sub test_data ($file) {
    my $file_dir = dirname(__FILE__);
    return catfile( $file_dir, 'data', $file );
}

sub new_pdf () {
    my $file = test_data('empty10.pdf');
    my ( undef, $tempfile ) = tempfile( DIR => $tempdir );

    copy( $file, $tempfile )
        or die "copy";
    return $tempfile;
}

sub new_tempfile () {
    my ( undef, $tempfile ) = tempfile( DIR => $tempdir );
    return $tempfile;
}
