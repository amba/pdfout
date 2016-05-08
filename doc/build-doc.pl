#!/usr/bin/env perl
use 5.010;
use warnings;
use strict;
use autodie;

use Pod::Simple::XHTML;

use File::Basename qw/basename/;
use File::Spec::Functions qw/catfile/;

my $prog_name = basename ($0);

sub state_usage {
    say "Usage: $prog_name [OPTIONS] DOC_DIR OUTPUT_DIR";
}

if (@ARGV != 2) {
    state_usage ();
    exit 1;
}

my ($doc_dir, $output_dir) = @ARGV;

if (not -d $output_dir) {
    die "directory '$output_dir' does not exist.\n";
}

my @files = glob catfile ($doc_dir, "*.pod");

foreach my $file (@files) {
    warn "parsing $file\n";

    my $parser = Pod::Simple::XHTML->new ();
    
    $parser->html_header_tags (
	    '<link rel="stylesheet" href="style.css" type="text/css" />');
    
    $parser->complain_stderr (1);
    
    my $out_file = catfile ($output_dir, basename ($file) =~ s/\.pod$/.html/r);
    
    warn "writing to $out_file\n";
    
    open (my $out_fh, ">", $out_file);
    
    $parser->output_fh ($out_fh);
    
    $parser->parse_file ($file);
    
    if ($parser->any_errata_seen ()) {
	die "file '$file' has pod errors\n";
    }
}
