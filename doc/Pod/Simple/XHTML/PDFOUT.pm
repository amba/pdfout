package Pod::Simple::XHTML::PDFOUT;

use 5.010;
use warnings;
use strict;

use parent qw/Pod::Simple::XHTML/;
sub new {
    my $self = shift;
    my $parser = $self->SUPER::new (@_);
    
    $parser->perldoc_url_prefix ('');
    $parser->complain_stderr (1);
    return $parser;
}

sub set_css {
    my $parser = shift;
    my $css = shift;
    
    $parser->html_header_tags (
	'<link rel="stylesheet" href="' . $css . '" type="text/css" />');
}

sub resolve_pod_page_link {
    my ($self, $to, $section) = @_;
    return undef unless defined $to || defined $section;

    my $link;
    if ($to && $section) {
	$link = "$to/$section";
    }
    elsif ($to) {
	$link = $to;
    }
    elsif ($section) {
	$link = $section;
    }
    
    my $num_pounds = $link =~ tr/#//;
    if ($num_pounds == 0) {
	return "$link.html";
    }
    elsif ($num_pounds == 1) {
	$link =~ /(.*)#(.*)/;
	my $file = $1;
	my $dest = $2;
	$dest = $self->idify($self->encode_entities($dest), 1);
	my $result = '#' . $dest;
	if ($file) {
	    $result = $file . '.html' . $result;
	}
	return $result;
    }
    else {
	die "too many '#' characters in '$link'";
    }
}

1;
