#!/usr/bin/env perl
use 5.020;
use warnings;
use strict;

use experimental 'signatures';
use experimental 'postderef';

use File::Spec::Functions qw/catfile/;

sub verbose_string ($verbose) {
    return $verbose ? "yes" : "no";
}

my @args = ('build', 1, '-g -O0 -Wall');
build_mupdf(@args);
build_pdfout(@args);
	    
sub build_mupdf ($out, $verbose, $cflags) {
    $out = catfile('..', $out, 'mupdf');
    my @command = (qw/make -C mupdf libs third build=debug/,
		   "verbose=$verbose", "OUT=$out", "XCFLAGS=$cflags",
		   "SYS_OPENSSL_CFLAGS= SYS_OPENSSL_LIBS=");

    safe_system(@command);
}

    
sub build_pdfout ($out, $verbose, $cflags, @make_args) {
    my @command = ('make', "OUT=$out", "CFLAGS=$cflags", @make_args);
    safe_system(@command);
}
    

sub safe_system(@command) {
    say "executing: @command";
    system(@command) == 0
	or die "command failed: $?";
}

    










	
