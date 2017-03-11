use 5.020;
use warnings;
use strict;

use experimental 'signatures';

my $build_folder = $ENV{APPVEYOR_BUILD_FOLDER};
if (not $build_folder) {
    die "no APPVEYOR_BUILD_FOLDER in ENV";
}
$build_folder =~ s{\\}{/}g;
warn "build folder: $build_folder\n";

safe_chdir($build_folder);

verbose_system(qw(perl --version));
verbose_system(qw(make --version));
verbose_system(qw(wget http://cpanmin.us -O /usr/bin/cpanm));
verbose_system(qw(chmod u+x /usr/bin/cpanm));

verbose_system(qw(./make.pl submodules));
verbose_system(qw(./make.pl --debug --verbose));

# Install test requirements.
verbose_system(qw(cpanm File::Slurper Text::Diff));

verbose_system(qw(./make.pl check));

sub verbose_system (@command) {
    warn <<"EOF";

##################################################
# command: @command
##################################################

EOF
    system(@command) == 0
        or die "command failed\n";
}

sub safe_chdir ($path) {
    chdir $path
        or die "cannot chdir to $path: $!";
}
