#!/usr/bin/env perl
use 5.020;
use warnings;
use strict;

use experimental 'signatures';
use experimental 'postderef';

use File::Spec::Functions qw/catfile/;
use Getopt::Long qw/:config gnu_getopt/;

use App::Prove;

my $command = shift @ARGV;

if (substr($command, 0, 1) eq '-') {
    unshift @ARGV, $command;
    $command = 'build';
}
elsif (not defined $command) {
    $command = 'build';
}


if ($command eq 'build') {
    build();
}
elsif ($command eq 'check') {
    check();
}
elsif ($command eq 'doc') {
    build_doc();
}
else {
    die "unkown command '$command'\n";
}


=head1 Subcommands

=head2 build

Build the mupdf libraries and the pdfout binary.

=cut

sub build {
    my $verbose;
    my $jobs = 1;
    my $cflags = '-g -O0 -Wall';
    
    GetOptions(
	'verbose|v' => \$verbose,
	'jobs|j=i' => \$jobs,
	'cflags|c=s' => \$cflags,
	)
	or die "invalid option\n";

    $verbose = verbose_string($verbose);
    
    my @args = ('build', $verbose, $cflags, $jobs);
    
    build_mupdf(@args);
    build_pdfout(@args);
}

sub verbose_string ($verbose) {
    return $verbose ? "yes" : "no";
}


sub build_mupdf ($out, $verbose, $cflags, $jobs) {
    $out = catfile('..', $out, 'mupdf');
    my @command = (qw/make -C mupdf libs third build=debug
                      SYS_OPENSSL_CFLAGS= SYS_OPENSSL_LIBS=/,
		   "-j$jobs", "verbose=$verbose", "OUT=$out",
		   "XCFLAGS=$cflags");

    safe_system(@command);
}


sub build_pdfout ($out, $verbose, $cflags, $jobs, @make_args) {
    my @command = ('make', "-j$jobs", "OUT=$out", "CFLAGS=$cflags",
		   @make_args);
    safe_system(@command);
}


sub safe_system(@command) {
    say "executing: @command";
    system(@command) == 0
	or die "command failed\n";
}

=head2 check

Test pdfout.

=cut
    
sub check {
    










	
