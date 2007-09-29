# Copyright 2007 Raphaël Hertzog <hertzog@debian.org>

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

package Dpkg::Path;

use strict;
use warnings;

use Exporter;
use Cwd qw(realpath);
our @ISA = qw(Exporter);
our @EXPORT_OK = qw(get_pkg_root_dir relative_to_pkg_root
		    guess_pkg_root_dir check_files_are_the_same);

=head1 NAME

Dpkg::Path - some common path handling functions

=head1 DESCRIPTION

It provides some functions to handle various path.

=head1 METHODS

=over 8

=item get_pkg_root_dir($file)

This function will scan upwards the hierarchy of directory to find out
the directory which contains the "DEBIAN" sub-directory and it will return
its path. This directory is the root directory of a package being built.

If no DEBIAN subdirectory is found, it will return undef.

=cut

sub get_pkg_root_dir($) {
    my $file = shift;
    $file =~ s{/+$}{};
    $file =~ s{/+[^/]+$}{} if not -d $file;
    while ($file) {
	return $file if -d "$file/DEBIAN";
	last if $file !~ m{/};
	$file =~ s{/+[^/]+$}{};
    }
    return undef;
}

=item relative_to_pkg_root($file)

Returns the filename relative to get_pkg_root_dir($file).

=cut

sub relative_to_pkg_root($) {
    my $file = shift;
    my $pkg_root = get_pkg_root_dir($file);
    if (defined $pkg_root) {
	$pkg_root .= "/";
	return $file if ($file =~ s/^\Q$pkg_root\E//);
    }
    return undef;
}

=item guess_pkg_root_dir($file)

This function tries to guess the root directory of the package build tree.
It will first use get_pkg_root_dir(), but it will fallback to a more
imprecise check: namely it will use the parent directory that is a
sub-directory of the debian directory.

It can still return undef if a file outside of the debian sub-directory is
provided.

=cut
sub guess_pkg_root_dir($) {
    my $file = shift;
    my $root = get_pkg_root_dir($file);
    return $root if defined $root;

    $file =~ s{/+$}{};
    $file =~ s{/+[^/]+$}{} if not -d $file;
    my $parent = $file;
    while ($file) {
	$parent =~ s{/+[^/]+$}{};
	last if not -d $parent;
	return $file if check_files_are_the_same("debian", $parent);
	$file = $parent;
	last if $file !~ m{/};
    }
    return undef;
}

=item check_files_are_the_same($file1, $file2)

This function verifies that both files are the same by checking that the device
numbers and the inode numbers returned by lstat() are the same.

=cut
sub check_files_are_the_same($$) {
    my ($file1, $file2) = @_;
    return 0 if ((! -e $file1) || (! -e $file2));
    my @stat1 = lstat($file1);
    my @stat2 = lstat($file2);
    my $result = ($stat1[0] == $stat2[0]) && ($stat1[1] == $stat2[1]);
    return $result;
}

=back

=head1 AUTHOR

Raphael Hertzog <hertzog@debian.org>.

=cut

1;