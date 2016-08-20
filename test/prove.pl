#!/usr/bin/env perl
use 5.020;
use warnings;
use strict;

use experimental 'signatures';
use experimental 'postderef';

use App::Prove;

use Getopt::Long qw/:config gnu_getopt/;

use File::Basename;
use File::Spec::Functions;
use Data::Dumper;

my $test_dir = dirname (__FILE__);

# Parse command line arguments.

my $valgrind;
my $jobs = 1;
my $tests;

GetOptions (
    "valgrind" => \$valgrind,
    "jobs|j=i" => \$jobs,
    "tests|t=s" => \$tests,
    );

my @argv = ($test_dir);

if ($tests) {
    @argv = split ' ', $tests;
    @argv = map catfile ($test_dir, $_), @argv;
}


my $attributes = {
    includes => [$test_dir],
    jobs => $jobs,
    argv => \@argv,
};

print Dumper($attributes);

if ($valgrind) {
    push $attributes->{test_args}->@*, '--valgrind';
}


my $prove = App::Prove->new($attributes);

exit ($prove->run () ? 0 : 1);
