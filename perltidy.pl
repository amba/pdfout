#!/usr/bin/env perl
use 5.020;
use warnings;
use strict;

use Perl::Tidy;
use File::Find;

my @files;

find(
    {
        wanted => sub {
            my $file = $_;
            if ( $file =~ /\.(pm|t|pl)$/ ) {
                push @files, $file;
            }
        },
        no_chdir => 1,
    },
    'test',
    'doc',
);

push @files, qw(make.pl perltidy.pl);

warn "running perltidy on files: ", join( "\n", @files ), "\n";
perltidy(
    perltidyrc => 'perltidyrc',
    argv       => [ '-b', '-bext=/', @files ],
);
