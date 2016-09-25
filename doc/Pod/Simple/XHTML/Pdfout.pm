package Pod::Simple::XHTML::Pdfout;

use 5.020;
use warnings;
use strict;

use experimental 'signatures';
use experimental 'postderef';

use Carp;
use Data::Dumper;
use parent qw/Pod::Simple::XHTML/;

sub new ($self, @params) {
    my $parser = $self->SUPER::new (@params);
    
    $parser->perldoc_url_prefix ('');
    $parser->complain_stderr (1);
    $parser->set_css('style.css');
    return $parser;
}

sub set_css ($parser, $css) {
    $parser->html_header_tags (
	'<link rel="stylesheet" href="' . $css . '" type="text/css" />');
}

sub resolve_pod_page_link ($self, $to, $section){
    if (not defined $to) {
	croak "no 'to' in link. section: $section";
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
	croak "too many '#' characters in '$to'";
    }
}

1;
