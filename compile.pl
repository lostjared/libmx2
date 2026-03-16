#!/usr/bin/env perl
use strict;
use warnings;
use Cwd 'abs_path';
use File::Basename;
use File::Path qw(make_path);

my $root = dirname(abs_path($0));
my $parent = dirname($root);
my $aciddrop_dir = "$parent/AcidDrop";
my $aciddrop_repo = "https://github.com/lostjared/AcidDrop.git";

my $nproc = `nproc 2>/dev/null` || `sysctl -n hw.ncpu 2>/dev/null` || 4;
chomp $nproc;

my %libmx2_flags = (
    "-DVULKAN" => "ON",
    "-DOPENGL" => "ON"
);

my $should_install = 0;
my $should_clean = 0;

for my $arg (@ARGV) {
   if($arg eq "clean") {
	$should_clean = 1;
   } elsif($arg eq "install") {
        $should_install = 1;
    } elsif ($arg =~ /^([^=]+)=(.*)$/) {
        $libmx2_flags{$1} = $2;
    }
}

sub run_cmd {
    my ($label, $cmd) = @_;
    print ">> [$label] $cmd\n";
    my $rc = system($cmd);
    if ($rc != 0) {
        die "!! [$label] Command failed (exit code: " . ($rc >> 8) . ")\n";
    }
}

sub build_libmx2 {
    my $build = "$root/build";
    my $source = "$root/libmx";
    die "Error: libmx2 source not found at $source\n" unless -d $source;

    make_path($build) unless -d $build;
    chdir($build) or die "Cannot cd to $build: $!\n";

    if($should_clean) { 
    	run_cmd("libmx2", "make clean");
    }
    my @flags;
    while (my ($k, $v) = each %libmx2_flags) { push @flags, "$k=$v"; }

    run_cmd("libmx2", "cmake -B . -S $source @flags");
    run_cmd("libmx2", "make -j$nproc");
    if($should_install) { 
    	run_cmd("libmx2", "sudo make install");
    }
}

build_libmx2();
