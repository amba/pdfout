#!/usr/bin/env perl
use 5.020;
use warnings;
use strict;

use experimental 'signatures';
use experimental 'postderef';

use File::Spec::Functions qw/catfile splitdir/;
use File::Copy;
use File::Find;
use File::Path qw/make_path remove_tree/;
use Getopt::Long qw/:config gnu_getopt/;
use List::Util qw/max/;

use Data::Dumper;
my %command_sub = (
    'build'      => \&build,
    'clean'      => \&clean,
    'check'      => \&check,
    'doc'        => \&build_doc,
    'upload-doc' => \&upload_doc,
    'submodules' => \&submodules,
    'cover'      => \&cover,
);

my $command = shift @ARGV;

if ( not defined $command ) {
    $command = 'build';
}
else {
    if ( $command eq '--help' || $command eq '-h' ) {
        print_help();
        exit 0;
    }

    if ( $command =~ /^-/ ) {
        unshift @ARGV, $command;
        $command = 'build';
    }
}

sub print_help {
    print <<'EOF';
Usage: ./make.pl SUBCOMMAND [OPTIONS]
   or: ./make.pl [OPTIONS]

Available subcommands:
EOF
    my @commands = sort keys %command_sub;
    say "  ", join( "\n  ", @commands );

    print <<'EOF';

If SUBCOMMAND is not given, it defaults to 'build'.

For a complete description of each subcommand, see
http://amba.github.io/pdfout/make.html.
EOF
}

if ( not exists $command_sub{$command} ) {
    die "unkown command '$command'\n";
}

$command_sub{$command}();

=head1 Building and testing pdfout
 
This document describes the details of pdfout's build system.

All build and test tasks are run with the F<make.pl> script. It supports the
following list of subcommands, which can also be viewed by running these
commands:

 ./make.pl --help
 or: ./make.pl -h

=head2 build

 ./make.pl build [OPTION]
 or: ./make.pl [OPTION]

Build the pdfout binary. In a first step, this will build the mupdf submodule.

The default build directory is F<build>.

 Options:
      --out=OUTPUT_DIR        alternative build directory.
      --cc=CC                 name of the C compiler
  -c, --cflags=CFLAGS         additional compiler flags
      --cppflags=CPPFLAGS     additional preprocessor flags
      --ldflags=LDFLAGS       additional linker flags
      --mupdf-cflags=CFLAGS   CFLAGS for mupdf. Defaults to '-O2 -g'
      --prefix=PREFIX         installation prefix
      --install               install pdfout
  -j, --jobs=JOBS             number of jobs used by make
  -v, --verbose               Show build commands

=cut

sub build {
    my $verbose;
    my $jobs          = 1;
    my $user_cflags   = '-O0';
    my $user_cppflags = '';
    my $cflags        = '-std=gnu99 -g -Wall';
    my $mupdf_cflags  = '-O2 -g';
    my $cc            = 'cc';
    my $out           = 'build';
    my $ldflags       = '';
    my $install;
    my $prefix = '/usr/local';

    GetOptions(
        'verbose|v'      => \$verbose,
        'jobs|j=i'       => \$jobs,
        'cflags|c=s'     => \$user_cflags,
        'cppflags=s'     => \$user_cppflags,
        'ldflags=s'      => \$ldflags,
        'mupdf-cflags=s' => \$mupdf_cflags,
        'cc=s'           => \$cc,
        'out=s'          => \$out,
        'install|i'      => \$install,
        'prefix=s'       => \$prefix,
    ) or die "invalid option\n";

    my $mupdf_build_dir   = catfile( $out,    'mupdf' );
    my $mupdf_include_dir = catfile( 'mupdf', 'include' );

    my $mupdf_ldlibs = [
        catfile( $mupdf_build_dir, 'libmupdf.a' ),
        catfile( $mupdf_build_dir, 'libmupdfthird.a' ),
    ];

    my $ldlibs = ['-lm'];

    my %args = (
        out          => $out,
        cc           => $cc,
        verbose      => $verbose,
        cflags       => "$cflags $user_cflags",
        cppflags     => "-Isrc -I$mupdf_include_dir $user_cppflags",
        jobs         => $jobs,
        ldflags      => $ldflags,
        ldlibs       => $ldlibs,
        mupdf_cflags => $mupdf_cflags,
        mupdf_ldlibs => $mupdf_ldlibs,

    );

    my @out_dirs
        = ( $out, catfile( $out, 'src' ), catfile( $out, 'src', 'program' ) );

    make_path(@out_dirs);

    build_mupdf(%args);
    my $binary = build_pdfout(%args);
    if ($install) {
        install( $binary, $prefix );
    }
}

sub install ( $binary, $prefix ) {
    my $bindir = catfile( $prefix, 'bin' );
    make_path($bindir);
    my $target = catfile( $bindir, 'pdfout' );
    copy( $binary, $target )
        or die "cannot copy '$binary' to '$target': $!";
}

sub build_mupdf (%args) {
    my $out = catfile( '..', $args{out}, 'mupdf' );
    my $verbose = verbose_string( $args{verbose} );
    safe_system(
        command => [
            qw/make -C mupdf libs third build=debug
                SYS_OPENSSL_CFLAGS= SYS_OPENSSL_LIBS=/,
            "-j$args{jobs}", "verbose=$verbose", "OUT=$out",
            "XCFLAGS=$args{mupdf_cflags}"
        ]
    );
}

sub build_pdfout (%args) {
    my @sources = glob('src/*.c src/program/*.c');
    my @objects;

    my @headers = glob('src/*.h src/program/*.h');

    for my $src (@sources) {
        my $obj = $src =~ s/c$/o/r;
        $obj = catfile( $args{out}, $obj );
        build_object_file(
            obj     => $obj,
            headers => \@headers,
            src     => $src,
            %args
        );
        push @objects, $obj;
    }

    my $binary = catfile( $args{out}, 'pdfout' );
    link_object_files( objects => \@objects, binary => $binary, %args );
    return $binary;
}

sub split_on_ws ($scalar) {

    # ' ' will emulate awk behavior: remove leading ws and split on /\s+/.
    return split ' ', $scalar;
}

sub build_object_file (%args) {
    my $obj     = $args{obj};
    my $src     = $args{src};
    my @headers = $args{headers}->@*;

    if ( not is_outdated( $obj, $src, @headers ) ) {
        return;
    }

    my @cflags   = split_on_ws( $args{cflags} );
    my @cppflags = split_on_ws( $args{cppflags} );

    my @command = (
        $args{cc}, '-o', $args{obj}, @cflags, @cppflags, '-c',
        $args{src}
    );
    my $msg;
    if ( $args{verbose} ) {
        $msg = "    @command";
    }
    else {
        $msg = "    CC $args{obj}";
    }

    safe_system(
        msg     => $msg,
        command => \@command
    );
}

sub link_object_files (%args) {
    my $binary  = $args{binary};
    my @objects = $args{objects}->@*;
    if ( not @objects ) {
        die "no object files given in link_object_files\n";
    }

    my @mupdf_ldlibs = $args{mupdf_ldlibs}->@*;
    my @ldlibs       = $args{ldlibs}->@*;

    if ( not is_outdated( $binary, @objects, @mupdf_ldlibs ) ) {
        return;
    }

    my @ldflags = split_on_ws( $args{ldflags} );

    my @command = (
        $args{cc}, '-o', $binary, @ldflags, @objects, @mupdf_ldlibs,
        @ldlibs
    );
    my $msg;
    if ( $args{verbose} ) {
        $msg = "    @command";
    }
    else {
        $msg = "    LD $binary";
    }

    safe_system(
        msg     => $msg,
        command => \@command
    );

    return $binary;
}

# Return true if $file needs update.
sub is_outdated ( $target, @deps ) {
    if ( not @deps ) {
        return;
    }

    if ( not -f $target ) {
        return 1;
    }

    my @mtimes;

    for my $dep (@deps) {
        my @stat = stat($dep);
        push @mtimes, $stat[9];
    }

    my $dep_time = max @mtimes;

    my $target_time = ( stat($target) )[9];

    if ( $dep_time > $target_time ) {
        return 1;
    }
    else {
        return;
    }
}

sub safe_system (%arg) {
    my $msg     = $arg{msg};
    my @command = $arg{command}->@*;

    if ( not defined $msg ) {
        $msg = "@command";
    }

    say $msg;
    system(@command) == 0
        or die "command failed\n";
}

sub verbose_string ($verbose) {
    return $verbose ? "yes" : "no";
}

=head2 clean

 ./make.pl clean [OPTION]

Clean the build directory. By default everything is cleaned. The options give
you more fine-grained control.

 Options:
      --out=OUTPUT_DIR        target directory
      --mupdf                 only clean the mupdf build
      --pdfout                keep the mupdf build
      --html                  only clean the doc output

=cut

use File::Path 'remove_tree';

sub clean {
    my $out = 'build';
    my $mupdf;
    my $pdfout;
    my $html;

    GetOptions(
        'out=s'  => \$out,
        'mupdf'  => \$mupdf,
        'pdfout' => \$pdfout,
        'html'   => \$html,
    ) or die "GetOptions";

    if ($mupdf) {
        remove_tree( catfile( $out, 'mupdf' ) );
    }

    if ($pdfout) {
        unlink( catfile( $out, 'pdfout' ) );
        remove_tree( catfile( $out, 'src' ) );
    }

    if ($html) {
        remove_tree( catfile( $out, 'html' ) );
    }

    if ( !( $mupdf || $pdfout || $html ) ) {
        remove_tree($out);
    }
}

=head2 check

 ./make.pl check
 or ./make.pl check --tests='info-dict.t page-count.t'

Test a pdfout build. This will run all F<*.t> files in the F<test> directory.

 Options:
      --out=OUTPUT_DIR        build directory
  -j, --jobs=JOBS             number of parallel jobs
      --valgrind              run tests under valgrind
  -t, --tests=TESTS           run only these tests
      --timer                 print elapsed time after each test

This requires L<Test::Files|https://metacpan.org/pod/Test::Files>. 

Using valgrind will only work, if the build does not use optimization.

=cut

sub check {
    my $test_dir = 'test';
    my $valgrind;
    my $jobs = 1;
    my $tests;
    my $out = 'build';
    my $timer;

    GetOptions(
        "jobs|j=i"  => \$jobs,
        "valgrind"  => \$valgrind,
        "tests|t=s" => \$tests,
        "out=s"     => \$out,
        "timer"     => \$timer,
    );

    my @argv = ($test_dir);

    if ($tests) {
        @argv = split ' ', $tests;
        @argv = map catfile( $test_dir, $_ ), @argv;
    }

    my $attributes = {
        includes  => [$test_dir],
        jobs      => $jobs,
        argv      => \@argv,
        test_args => [ '--pdfout', catfile( $out, 'pdfout' ) ],
        timer     => $timer
    };

    if ($valgrind) {
        push $attributes->{test_args}->@*, '--valgrind';
    }
    require App::Prove;
    my $prove = App::Prove->new($attributes);

    exit( $prove->run() ? 0 : 1 );
}

=head2 doc

 ./make.pl doc

Build the documentation. This will produce nicely formatted XHTML. By default,
the output will be written into F<build/html/>.

This requires 
L<Pod::Simple::XHTML|https://metacpan.org/pod/Pod::Simple::XHTML>.

 Options:
      --out=OUTPUT_DIR        put output into OUTPUT_DIR/html

=cut

use lib 'doc';

sub build_doc {
    my $out = 'build';
    GetOptions( 'out|o=s' => \$out );

    $out = catfile( $out, 'html' );
    generate_doc($out);
}

sub generate_doc ($out) {
    require Pod::Simple::XHTML::Pdfout;
    make_path($out);
    copy( catfile( 'doc', 'style.css' ), catfile( $out, 'style.css' ) )
        or die "cannot copy style.css $!";

    my @files = qw/
        make.pl
        README.pod
        /;

    find(
        {
            wanted => sub {
                my $file = $_;
                if ( -f $file && $file =~ /\.pod$/ ) {
                    push @files, $file;
                }
            },
            no_chdir => 1,
        },
        'doc'
    );

    for my $file (@files) {
        pod_to_html( $out, $file );
    }
}

sub pod_to_html ( $out, $file ) {
    my @split = splitdir $file;
    if ( $split[0] eq 'doc' ) {
        shift @split;
    }
    $split[-1] =~ s/\.(pl|pm|pod|t)$/.html/;
    my $html = catfile( $out, join( '-', @split ) );
    warn "processing: file: $file, html: $html\n";

    open my $fh, '>', $html
        or die "cannot open '$html': $!";

    my $parser = Pod::Simple::XHTML::Pdfout->new();
    $parser->output_fh($fh);
    $parser->parse_file($file);
    if ( $parser->any_errata_seen() ) {
        die "file '$file' has pod errors\n";
    }
}

sub safe_chdir ($dir) {
    chdir $dir
        or die "cannot chdir to '$dir': $!";
}

=head2 upload-doc

 ./make.pl upload-doc

Maintainer command to update the docs at L<https://amba.github.io/pdfout>.

=cut

sub upload_doc {
    my $out = '/home/simon/amba.github.io/pdfout';
    remove_tree($out);
    generate_doc($out);
    safe_chdir $out;
    safe_system( command => [ qw/git commit -am/, "update pdfout docs" ] );
    safe_system( command => [qw/git push/] );
}

=head2 submodules

 ./make.pl submodules

Check out the mupdf submodule and mupdf's own submodules.

=cut

sub submodules {
    say 'checking out git submodules';

    safe_system( command => [qw/git submodule update --init/] );

    safe_chdir 'mupdf';

    # We do not need the curl submodule.
    my @mupdf_submodles = qw(
        thirdparty/freetype
        thirdparty/jbig2dec
        thirdparty/jpeg
        thirdparty/openjpeg
        thirdparty/zlib
        thirdparty/mujs
        thirdparty/harfbuzz
    );

    safe_system(
        command => [ qw/git submodule update --init/, @mupdf_submodles ] );
}

=head2 cover

 ./make.pl cover -j4

Requires L<Devel::Cover|https://metacpan.org/pod/Devel::Cover> installed.

Do coverage build with gcov in F<cover-build>.

Open output of gcov2perl in firefox.

 Options:
  -j, --jobs=JOBS             number of parallel jobs

This requires L<Test::Files|https://metacpan.org/pod/Test::Files>. 

Using valgrind will only work, if the build does not use optimization.

=cut

sub cover {
    my $jobs = 1;

    GetOptions( "jobs|j=i" => \$jobs, );

    safe_system(
        command => [
            './make.pl',
            'build',
            '--cflags=-fprofile-arcs -ftest-coverage',
            '--ldflags=-fprofile-arcs -ftest-coverage',
            '--out=cover-build',
            '--mupdf-cflags=-O0 -g',
            "-j$jobs"
        ]
    );

    safe_system(
        command => [ './make.pl', 'check', '--out=cover-build', "-j$jobs" ] );

    safe_system( command =>
            [ 'gcov', '--object-directory=cover-build/src', glob('src/*.c') ]
    );

    safe_system(
        command => [
            'gcov', '--object-directory=cover-build/src/program',
            glob('src/program/*.c')
        ]
    );

    my @gcovs = glob('*.gcov');
    safe_system(
        command => [ 'gcov2perl', '-db', 'cover_db', @gcovs ] );

    safe_system( command => ['cover'] );

    safe_system( command => [qw(firefox -new-tab cover_db/coverage.html)] );

    for my $gcov (@gcovs) {
	unlink($gcov);
    }
}

