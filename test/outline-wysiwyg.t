#!/usr/bin/env perl
use warnings;
use strict;
use utf8;
use 5.020;

use Test::Pdfout::Command tests => 20;
use Test::More;
use Testlib;

set_get_test(
    command => [ 'outline', '--wysiwyg' ],

    broken_input => [
        '1',
        'title1',
        'd=1 .',
        'title 1 .',
        'title 2147483648',    # INT_MAX overflow
        '    abc 1',           # too mutch indent
        "abc 1\n        def 2",
        '. . . . . . 1',       # empty title
    ],

    input => <<"EOD",
title1 1 
ζβσ \t2
3 3\t
    4 4
    5 5
        6 6
        7 7
            8 8
\t      
\t    d=1  

    9 9
EOD
    expected => <<'EOD',
title1 1
ζβσ 2
3 3
    4 4
    5 5
        6 6
        7 7
            8 8
    9 10
EOD
    empty => "",
);
