#!/usr/bin/perl
# vim: set ts=2 sw=2 tw=99 noet: 

use strict;
use Cwd;
use File::Basename;
use File::Path;

my ($myself, $path) = fileparse($0);
chdir($path);

require 'helpers.pm';

#Go back above build dir
chdir(Build::PathFormat('../..'));

#Get the source path.
our ($root) = getcwd();

#update and configure shiz
if ($^O eq "darwin") {
	$ENV{'SOURCEMOD18'} = '/Users/builds/slaves/common/sourcemod-1.8';
}

rmtree('OUTPUT');
mkdir('OUTPUT') or die("Failed to create output folder: $!\n");
chdir('OUTPUT');
my ($result);

#configure AMBuild
if ($^O eq "linux") {
	$result = `CC=clang-3.8 CXX=clang-3.8 python ../build/configure.py --enable-optimize`;
} elsif ($^O eq "darwin") {
	$result = `CC=clang CXX=clang python ../build/configure.py --enable-optimize`;
} else {
	$result = `C:\\Python31\\Python.exe ..\\build\\configure.py --enable-optimize`;
}
print "$result\n";
if ($? != 0) {
	die('Could not configure!');
}
