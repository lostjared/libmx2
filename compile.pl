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

for my $arg (@ARGV) {
    if ($arg =~ /^([^=]+)=(.*)$/) {
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

sub clone_aciddrop {
    chdir($parent) or die "Cannot cd to $parent: $!\n";
    if (-d "AcidDrop") {
        run_cmd("rm", "rm -rf AcidDrop");
    }
    run_cmd("git", "git clone $aciddrop_repo");
    run_cmd("git", "git config --global --add safe.directory $aciddrop_dir");
}

sub build_libmx2 {
    my $build = "$root/build";
    my $source = "$root/libmx";
    die "Error: libmx2 source not found at $source\n" unless -d $source;

    make_path($build) unless -d $build;
    chdir($build) or die "Cannot cd to $build: $!\n";

    my @flags;
    while (my ($k, $v) = each %libmx2_flags) { push @flags, "$k=$v"; }

    run_cmd("libmx2", "cmake -B . -S $source @flags");
    run_cmd("libmx2", "make -j$nproc");
    #run_cmd("libmx2", "sudo make install");
}

sub build_aciddrop {
    unless (-d $aciddrop_dir && -f "$aciddrop_dir/CMakeLists.txt") {
        clone_aciddrop();
    }
    die "Error: AcidDrop source not found at $aciddrop_dir\n" unless -d $aciddrop_dir;

    my $build = "$aciddrop_dir/build";
    make_path($build) unless -d $build;
    chdir($build) or die "Cannot cd to $build: $!\n";

    run_cmd("AcidDrop", "cmake -B . -S $aciddrop_dir");
    run_cmd("AcidDrop", "make -j$nproc");
}

build_libmx2();
build_aciddrop();
