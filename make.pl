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

my $command = shift @ARGV;

if (not defined $command) {
    $command = 'build';
}
elsif (substr($command, 0, 1) eq '-') {
    unshift @ARGV, $command;
    $command = 'build';
}

my %command_sub = (
    'build' => \&build,
    'clean' => \&clean,
    'check' => \&check,
    'doc'   => \&build_doc,
    'submodules' => \&submodules,
    );

if (not exists $command_sub{$command}) {
        die "unkown command '$command'\n";
}

$command_sub{$command}();

=head1 Building and testing pdfout

=head2 build

Build the mupdf libraries and the pdfout binary.

=cut

sub build {
    my $verbose;
    my $jobs = 1;
    my $user_cflags = '-O0';
    my $user_cppflags = '';
    my $cflags = '-std=gnu99 -g -Wall';
    my $cc = 'cc';
    my $out = 'build';
    my $ldflags = '';
    my $install;
    my $prefix = '/usr/local';
    
    GetOptions(
	'verbose|v' => \$verbose,
	'jobs|j=i' => \$jobs,
	'cflags|c=s' => \$user_cflags,
	'cppflags=s' => \$user_cppflags,
	'cc=s' => \$cc,
	'out=s' => \$out,
	'install|i' => \$install,
	'prefix=s' => \$prefix,
	)
	or die "invalid option\n";

    my $mupdf_build_dir = catfile($out, 'mupdf');
    my $mupdf_include_dir = catfile('mupdf', 'include');

    my $mupdf_ldlibs = [
	catfile($mupdf_build_dir, 'libmupdf.a'),
	catfile($mupdf_build_dir, 'libmupdfthird.a'),
	];
	
    my $ldlibs = ['-lm'];
    
    my %args = (
	out => $out,
	cc => $cc,
	verbose => $verbose,
	cflags =>  "$cflags $user_cflags",
	cppflags => "-Isrc -I$mupdf_include_dir $user_cppflags",
	jobs => $jobs,
	ldflags => $ldflags,
	ldlibs => $ldlibs,
	mupdf_ldlibs => $mupdf_ldlibs,
	
	);

    my @out_dirs = ($out, catfile($out, 'src'),
		    catfile($out, 'src', 'program'));

    make_path(@out_dirs);
    
    build_mupdf(%args);
    my $binary = build_pdfout(%args);
    if ($install) {
	install($binary, $prefix);
    }
}

sub install ($binary, $prefix) {
    my $bindir = catfile($prefix, 'bin');
    make_path($bindir);
    my $target = catfile($bindir, 'pdfout');
    copy($binary, $target)
	or die "cannot copy '$binary' to '$target': $!";
}

sub build_mupdf (%args) {
    my $out = catfile('..', $args{out}, 'mupdf');
    my $verbose = verbose_string($args{verbose});
    my @command = (qw/make -C mupdf libs third build=debug
                      SYS_OPENSSL_CFLAGS= SYS_OPENSSL_LIBS=/,
		   "-j$args{jobs}", "verbose=$verbose", "OUT=$out",
		   "XCFLAGS=$args{cflags}");
    
    safe_system("@command", @command);
}

sub build_pdfout (%args) {
    my @sources = glob('src/*.c src/program/*.c');
    my @objects;
    for my $src (@sources) {
	my $obj = $src =~ s/c$/o/r;
	$obj = catfile($args{out}, $obj);
	build_object_file(obj => $obj, src => $src, %args);
	push @objects, $obj;
    }

    my $binary = catfile($args{out}, 'pdfout');
    link_object_files (objects => \@objects, binary => $binary, %args);
    return $binary;
}

sub split_on_ws ($scalar) {
    # ' ' will emulate awk behavior: remove leading ws and split on /\s+/.
    return split ' ', $scalar;
}

sub build_object_file (%args) {
    my $obj = $args{obj};
    my $src = $args{src};

    if (not is_outdated($obj, $src)) {
	return;
    }
    
    my @cflags = split_on_ws($args{cflags});
    my @cppflags = split_on_ws($args{cppflags});
    
    my @command = ($args{cc}, '-o', $args{obj}, @cflags, @cppflags, '-c',
		   $args{src});
    my $msg;
    if ($args{verbose}) {
	$msg = "    @command";
    }
    else {
	$msg = "    CC $args{obj}";
    }
    safe_system($msg, @command);
}

sub link_object_files (%args) {
    my $binary = $args{binary};
    my @objects = $args{objects}->@*;
    if (not @objects) {
	die "no object files given in link_object_files\n";
    }

    my @mupdf_ldlibs = $args{mupdf_ldlibs}->@*;
    my @ldlibs = $args{ldlibs}->@*;
    
    if (not is_outdated($binary, @objects, @mupdf_ldlibs)) {
	return;
    }
    
    my @ldflags = split_on_ws($args{ldflags});

    my @command = ($args{cc}, '-o', $binary, @ldflags, @objects,
		   @mupdf_ldlibs, @ldlibs);
    my $msg;
    if ($args{verbose}) {
	$msg = "    @command";
    }
    else {
	$msg = "    LD $binary";
    }
    safe_system($msg, @command);
    return $binary;
}

# Return true if $file needs update.
sub is_outdated ($target, @deps) {
    if (not @deps) {
	return;
    }
    
    if (not -f $target) {
	return 1;
    }
    
    my @mtimes;
    
    for my $dep (@deps) {
	my @stat = stat($dep);
	push @mtimes, $stat[9];
    }

    my $dep_time = max @mtimes;
    
    my $target_time = (stat($target))[9];

    if ($dep_time > $target_time) {
	return 1;
    }
    else {
	return;
    }
}


sub safe_system ($msg, @command) {
    say $msg;
    system(@command) == 0
	or die "command failed\n";
}


sub verbose_string ($verbose) {
    return $verbose ? "yes" : "no";
}

=head2 clean

=cut

use File::Path 'remove_tree';

sub clean {    
    my $out = 'build';
    my $mupdf;
    my $pdfout;
    
    GetOptions(
	'out=s' => \$out,
	'mupdf' => \$mupdf,
	'pdfout' => \$pdfout,
	) or die "GetOptions";

    
    if ($mupdf) {
	remove_tree(catfile($out, 'mupdf'));
    }
    
    if ($pdfout) {
	unlink(catfile($out, 'pdfout'));
	remove_tree(catfile($out, 'src'));
    }

    if (!($mupdf || $pdfout)) {
	remove_tree($out);
    }
}

=head2 check

Test pdfout.

=cut

sub check {
    my $test_dir = 'test';
    my $valgrind;
    my $jobs = 1;
    my $tests;
    my $out = 'build';
    
    GetOptions (
	"valgrind" => \$valgrind,
	"jobs|j=i" => \$jobs,
	"tests|t=s" => \$tests,
	"out=s" => \$out,
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
	test_args => ['--pdfout', catfile($out, 'pdfout')],
    };

    if ($valgrind) {
	push $attributes->{test_args}->@*, '--valgrind';
    }
    require App::Prove;
    my $prove = App::Prove->new($attributes);

    exit ($prove->run() ? 0 : 1);
}
    
#
# doc
#

use lib 'doc';

sub build_doc {
    require Pod::Simple::XHTML::Pdfout;

    my $out = 'build';
    GetOptions(
	'out|o=s' => \$out
	);

    $out = catfile($out, 'html');
    make_path($out);
    copy(catfile('doc', 'style.css'), catfile($out, 'style.css'))
	or die "cannot copy style.css $!";
    
    my @files = qw/
                     make.pl
/;

    find({
	wanted => sub {
	    my $file = $_;
	    if (-f $file && $file =~ /\.pod$/) {
		push @files, $file;
	    }
	},
	no_chdir => 1,
	 },
	 'doc');

    for my $file (@files) {
	pod_to_html($out, $file);
    }
}

sub pod_to_html ($out, $file) {
    my @split = splitdir $file;
    if ($split[0] eq 'doc') {
	shift @split;
    }
    $split[-1] =~ s/\.(pl|pm|pod|t)$/.html/;
    my $html = catfile($out, join('-', @split));
    warn "processing: file: $file, html: $html\n";

    open my $fh, '>', $html
	or die "cannot open '$html': $!";

    my $parser = Pod::Simple::XHTML::Pdfout->new();
    $parser->output_fh($fh);
    $parser->parse_file($file);
    if ($parser->any_errata_seen()) {
	die "file '$file' has pod errors\n";
    }
}


sub submodules {
    say 'checking out git submodules';

    my @command = qw/git submodule update --init/;
    safe_system ("@command", @command);
    
    chdir 'mupdf'
	or die 'cannot change directory to mupdf';

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

    @command = (qw/git submodule update --init/, @mupdf_submodles);
    safe_system ("@command", @command);
}

	
