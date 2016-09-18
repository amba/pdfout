package Pod::Simple::XHTML::PDFOUT;

use 5.010;
use warnings;
use strict;
use Carp;

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

sub resolve_pod_page_link ($self, $to, $section){
    if (not defined $to) {
	croak "missing 'to' in link";
    }

    if (defined $section) {
	croak "'section' not allowed in link";
    }
    
    
    my $num_pounds = $to =~ tr/#//;
    if ($num_pounds == 0) {
	return "$to.html";
    }
    elsif ($num_pounds == 1) {
	$to =~ /(.*)#(.*)/;
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
	die "too many '#' characters in '$to'";
    }
}

1;
