package Testlib;

use 5.010;
use warnings;
use strict;

use File::Spec::Functions;
use File::Copy;

sub new_pdf {
    my $file = catfile('data', 'empty10.pdf');
    
