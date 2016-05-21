#!/usr/bin/env perl
use 5.010;
use warnings;
use strict;
use autodie;

use Pod::Simple::XHTML::PDFOUT;

use File::Find;
use File::Basename qw/basename/;
use File::Path qw/mkpath/;
use File::Copy;
use File::Spec::Functions qw/catfile catdir abs2rel splitpath/;

use Getopt::Long;

my $prog_name = basename ($0);
sub state_usage {
    say "Usage: $prog_name [OPTIONS] DOC_DIR";
}

my $doc_dir;
my $output_dir = 'html';



sub gen_dirs {
    mkpath ($output_dir);
    foreach my $file (@_) {
	my $relative_path = abs2rel ($file, $doc_dir);
	my (undef, $dir, undef) = splitpath ($relative_path);
	if ($dir) {
	    my $output = catdir ($output_dir, $dir);
	    mkpath ($output);
	}
    }
}

sub process_file {
    my $file = shift;
    
    warn "parsing $file\n";

    my $parser = Pod::Simple::XHTML::PDFOUT->new ();
    
    my $out_file = abs2rel ($file, $doc_dir);
    warn "abs2rel ($file, $doc_dir) = $out_file";

    my $subdir_level = $out_file =~ tr|/||;

    my $css = '../' x $subdir_level . 'style.css';
    $parser->set_css ($css);
    
    $out_file =~ s/\.pod$/.html/;
    $out_file = catfile ($output_dir, $out_file);
    
    warn "writing to $out_file\n";
    
    open (my $out_fh, ">", $out_file);
    
    $parser->output_fh ($out_fh);

    $parser->parse_file ($file);
    
    if ($parser->any_errata_seen ()) {
	die "file '$file' has pod errors\n";
    }
}

sub build {
    my @files;
    File::Find::find({
	wanted => sub {
	    if (-f && /pod$/) {
		push @files, $_;
	    }
	},
	no_chdir => 1}, $doc_dir);
    gen_dirs ($output_dir, @files);
    my $css = catfile ($doc_dir, "style.css");
    copy ($css, catfile ($output_dir, "style.css"));

    for my $file (@files) {
	process_file($file);
    }
}

sub assert_argv {
    my $num = shift;
    if (@ARGV != $num) {
	state_usage ();
	exit (1);
    }
}
my $mode_clean;
my $mode_build;
my $mode_help;

GetOptions ("build" => \$mode_build,
	    "clean" => \$mode_clean,
	    "help|h" => \$mode_help,
    ) or exit 1;

if ($mode_help) {
    state_help ();
    exit (0);
}

if ($mode_build) {
    assert_argv (1);
    $doc_dir = shift @ARGV;
    build ();
}

if ($mode_clean) {
    File::Find::find({wanted => sub {
	if (-f && /(\.html|\.css)$/) {
	    unlink ($File::Find::name);
	}
		      },
		      no_chdir => 1,
		     }, 'html');
}

	
    
    

