#!/usr/bin/env perl
use strict;
use warnings;
use Cwd 'abs_path';
use File::Basename;
use File::Path qw(make_path);

my $root = dirname(abs_path($0));
my $aciddrop_dir = dirname($root) . "/AcidDrop";
my $nproc = `nproc 2>/dev/null` || 4;
chomp $nproc;

sub run_cmd {
    my ($label, $cmd) = @_;
    print ">> [$label] $cmd\n";
    my $rc = system($cmd);
    if ($rc != 0) {
        die "!! [$label] Command failed (exit code: " . ($rc >> 8) . ")\n";
    }
}

sub build_libmx2 {
    print "=" x 60, "\n";
    print "  Building libmx2 (Vulkan=ON, OpenGL=OFF)\n";
    print "=" x 60, "\n\n";

    my $build = "$root/build";
    my $source = "$root/libmx";

    die "Error: libmx2 source not found at $source\n" unless -d $source;

    make_path($build) unless -d $build;
    chdir($build) or die "Cannot cd to $build: $!\n";

    run_cmd("libmx2", "cmake -B . -S $source -DVULKAN=ON -DOPENGL=OFF");
    run_cmd("libmx2", "make -j$nproc");

    print "\n>> [libmx2] Installing...\n";
    run_cmd("libmx2", "sudo make install");

    print "\n>> [libmx2] Build and install complete.\n\n";
}

sub build_aciddrop {
    print "=" x 60, "\n";
    print "  Building AcidDrop\n";
    print "=" x 60, "\n\n";

    die "Error: AcidDrop source not found at $aciddrop_dir\n" unless -d $aciddrop_dir;

    my $build = "$aciddrop_dir/build";
    make_path($build) unless -d $build;
    chdir($build) or die "Cannot cd to $build: $!\n";

    run_cmd("AcidDrop", "cmake -B . -S $aciddrop_dir");
    run_cmd("AcidDrop", "make -j$nproc");

    print "\n>> [AcidDrop] Build complete.\n";
    print ">> [AcidDrop] Executable: $build/AcidDrop\n\n";
}

# --- Main ---
print "\n";
print "=" x 60, "\n";
print "  libmx2 + AcidDrop Build Script\n";
print "=" x 60, "\n\n";

build_libmx2();
build_aciddrop();

print "=" x 60, "\n";
print "  All builds finished successfully!\n";
print "=" x 60, "\n";
