package Test::Pdfout::Command;

use 5.010;
use warnings;
use strict;

use Carp;
use IPC::Open3;
use Text::Diff;
use File::Spec::Functions qw/catfile/;

use parent 'Test::Builder::Module';

our @EXPORT = qw/
pdfout_ok
/;

my $class = __PACKAGE__;
my $builder = $class->builder;
binmode $builder->output,         ":encoding(utf8)";
binmode $builder->failure_output, ":encoding(utf8)";
binmode $builder->todo_output,    ":encoding(utf8)";

sub close_fh {
    my $fh = shift;
    close $fh
	or croak "cannot close filehandle";
}

sub string_ok {
    my ($expected, $got, $name) = @_;
    my $tb = $class->builder;

    if (not $tb->ok($got eq $expected, $name)) {
	my $diff = diff(\$expected, \$got, {
	    STYLE => 'Table',
	    FILENAME_A => 'expected',
	    FILENAME_B => 'received',
			});
	$tb->diag($diff);
	return 0;
    }
    return 1;
}

sub output_ok {
    my ($expected_out, $out_fh) = @_;
    if (not $expected_out) {
	return 1;
    }
    my $output = do { local $/; <$out_fh>};
    if (ref $expected_out) {
	my $tb = $class->builder;
	return $tb->like($output, $expected_out, "output matches regexp");
    }
    else {
	return string_ok($expected_out, $output, "output as expected");
    }
}

sub signal_name {
    my $signal = shift;
    return
	$signal == 6 ? "SIGABRT" :
	$signal == 9 ? "SIGKILL" :
	$signal == 11 ? "SIGSEGV" :
	$signal == 13 ? "SIGPIPE" : $signal;
}

sub status_ok {
    my ($wait_status, $expected_status) = @_;

    my $tb = $class->builder;
    
    if (not defined $expected_status) {
	$expected_status = 0;
    }
    
    my $child_exit_status = $wait_status >> 8;

    my $signal = $wait_status & 127;
    if (not $tb->ok($signal == 0, "child not killed by signal")) {
	my $signal_name = signal_name($signal);
	$tb->diag("child died by signal $signal_name");
	return 0;
    }
    
    return $tb->is_num($child_exit_status, $expected_status, "exit status");
}
    
sub command_subtest {
    my $tb = $class->builder;
    my %args = @_;

    local $Test::Builder::Level = $Test::Builder::Level + 4;
    
    my ($in_fh, $out_fh, $err_fh);
    use Symbol 'gensym'; $err_fh = gensym;

    if (ref $args{command} ne 'ARRAY') {
	croak "argument 'command' must be an arrayref";
    }
    
    my $pid = open3($in_fh, $out_fh, $err_fh, @{$args{command}});
    binmode($in_fh, ':utf8');
    binmode($out_fh, ':utf8');
    binmode($err_fh, ':utf8');
    
    my $input = $args{input};
    if ($input) {
	$tb->diag("sending input...");
    }
    if ($input) {
	print {$in_fh} $input;
    }

    # Close write end of the pipe, so that the command sees end-of-file.
    close_fh($in_fh);

    my $retval = 1;

    # Do not close out_fh and err_fh before waitpid, or the command can get
    # SIGPIPE.

    $tb->ok(waitpid($pid, 0) != -1, "waitpid found child process");
    
    status_ok ($?, $args{status});
	
    $retval &&= output_ok($args{expected_out}, $out_fh);

    my $err_msg = do {local $/; <$err_fh>};
    if ($err_msg) {
	$tb->diag("err_msg: $err_msg");
    }
    
    close_fh($out_fh);
    close_fh($err_fh);

    return $retval;
}

sub command_ok {
    my $tb = $class->builder;
    
    if (ref $_[0] or @_ % 2 != 0) {
	croak "command_ok needs a flat hash argument";
    }
    my %args = @_;
    my $name = "@{$args{command}}";
    return $tb->subtest($name, \&command_subtest, %args);
}

sub pdfout_ok {
    local $Test::Builder::Level = $Test::Builder::Level + 1;
    my %args = @_;
    unshift @{$args{command}}, './pdfout';
    return command_ok(%args);
}

1;