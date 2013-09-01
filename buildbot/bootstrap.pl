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

rmtree('OUTPUT');
mkdir('OUTPUT') or die("Failed to create output folder: $!\n");
chdir('OUTPUT');
my ($result);
print "Attempting to reconfigure...\n";

#update and configure shiz
if ($^O eq "linux") {
	#my @sdks = ('sourcemod-1.3', 'mmsource-1.8', 'hl2sdk', 'hl2sdk-ob', 'hl2sdk-ob-valve', 'hl2sdk-l4d', 'hl2sdk-l4d2');
	#my ($sdk);
	#foreach $sdk (@sdks) {
	#	print "Updating checkout of ", $sdk, " on ", $^O, "\n";
	#	$result = `hg pull -u /home/builds/common/$sdk`;
	#	print $result;
	#}
	
	$ENV{'SOURCEMOD14'} = '/home/builds/common/sourcemod-1.4';
	$ENV{'SMCENTRAL'} = '/home/builds/common/sourcemod-central';
	$ENV{'MMSOURCE19'} = '/home/builds/common/mmsource-1.9';
	
	$ENV{'HL2SDK'} = '/home/builds/common/hl2sdk';
	$ENV{'HL2SDKOB'} = '/home/builds/common/hl2sdk-ob';
	$ENV{'HL2SDKOBVALVE'} = '/home/builds/common/hl2sdk-ob-valve';
	$ENV{'HL2SDKL4D'} = '/home/builds/common/hl2sdk-l4d';
	$ENV{'HL2SDKL4D2'} = '/home/builds/common/hl2sdk-l4d2';
	$ENV{'HL2SDKCSGO'} = '/home/builds/common/hl2sdk-csgo';
} elsif ($^O eq "darwin") {
	#my @sdks = ('sourcemod-1.3', 'mmsource-1.8', 'hl2sdk-ob-valve');
	#my ($sdk);
	#foreach $sdk (@sdks) {
	#	print "Updating checkout of ", $sdk, " on ", $^O, "\n";
	#	$result = `hg pull -u /Users/builds/slaves/common/$sdk`;
	#	print $result;
	#}
	
	$ENV{'SOURCEMOD14'} = '/Users/builds/slaves/common/sourcemod-1.4';
	$ENV{'SMCENTRAL'} = '/Users/builds/slaves/common/sourcemod-central';
	$ENV{'MMSOURCE18'} = '/Users/builds/slaves/common/mmsource-1.8';
	$ENV{'MMSOURCE19'} = '/Users/builds/slaves/common/mmsource-central';
	
	$ENV{'HL2SDKOBVALVE'} = '/Users/builds/slaves/common/hl2sdk-ob-valve';
	$ENV{'HL2SDKCSS'} = '/Users/builds/slaves/common/hl2sdk-css';
} else {
	#JUST IN CASE
	#$ENV{'MMSOURCE19'} = "C:\\Scripts\\common\\mmsource-1.9";
	#$ENV{'HL2SDKCSGO'} = "C:\\Scripts\\common\\hl2sdk-csgo";
}
# Windows vars should already be set.

#configure AMBuild
if ($^O eq "linux") {
	$result = `CC=gcc CXX=gcc python3 ../build/configure.py --enable-optimize`;
} elsif ($^O eq "darwin") {
	$result = `CC=clang CXX=clang python3 ../build/configure.py --enable-optimize`;
} else {
	$result = `C:\\Python31\\Python.exe ..\\build\\configure.py --enable-optimize`;
}
print "$result\n";
if ($? != 0) {
	die('Could not configure!');
}
