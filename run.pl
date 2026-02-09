#!/usr/bin/env perl
use strict;
use warnings;
use Cwd 'abs_path';
use File::Basename;

my $root = dirname(abs_path($0));
my $parent = dirname($root);
my $build_dir = "$root/build";
my $source_dir = "$root/libmx";

my $program = shift @ARGV;

unless ($program) {
    print "Usage: ./run <program_name> [extra args...]\n\n";
    print "Available programs:\n";

    # List programs from the main build directory
    opendir(my $dh, $build_dir) or die "Cannot open $build_dir: $!\n";
    my @progs;
    while (my $entry = readdir($dh)) {
        next if $entry =~ /^\./;
        my $exe = "$build_dir/$entry/$entry";
        if (-x $exe) {
            push @progs, $entry;
        } elsif (-d "$build_dir/$entry") {
            # Check for any executable in the subdirectory
            opendir(my $sh, "$build_dir/$entry");
            while (my $f = readdir($sh)) {
                next if $f =~ /^\./;
                if (-f "$build_dir/$entry/$f" && -x "$build_dir/$entry/$f") {
                    push @progs, $entry;
                    last;
                }
            }
            closedir($sh);
        }
    }
    closedir($dh);

    # Check for standalone project directories with their own build/
    for my $base ($root, $parent) {
        opendir(my $rh, $base) or next;
        while (my $entry = readdir($rh)) {
            next if $entry =~ /^\./;
            next if $entry eq 'build' || $entry eq 'libmx';
            my $exe = "$base/$entry/build/$entry";
            push @progs, $entry if -x $exe;
        }
        closedir($rh);
    }

    my %seen;
    for my $p (sort grep { !$seen{$_}++ } @progs) {
        print "  $p\n";
    }
    exit 1;
}

# First check the main build directory (build/<name>/<name>)
my $exe = "$build_dir/$program/$program";
my $data = "$source_dir/$program";

# If exact match not found, search for any executable in the build subdirectory
if (!-x $exe && -d "$build_dir/$program") {
    opendir(my $bh, "$build_dir/$program");
    while (my $f = readdir($bh)) {
        next if $f =~ /^\./;
        my $candidate = "$build_dir/$program/$f";
        if (-f $candidate && -x $candidate) {
            $exe = $candidate;
            last;
        }
    }
    closedir($bh);
}

if (-x $exe && -d $data) {
    my $exe_name = basename($exe);
    chdir("$build_dir/$program") or die "Cannot cd to $build_dir/$program: $!\n";
    my @cmd = ("./$exe_name", "-p", $data, @ARGV);
    print ">> @cmd\n";
    exec(@cmd) or die "Failed to exec $exe_name: $!\n";
}

# Check standalone project directories (inside libmx2/ or sibling ../)
for my $base ($root, $parent) {
    my $standalone_exe = "$base/$program/build/$program";
    my $standalone_data = "$base/$program";

    if (-x $standalone_exe && -d $standalone_data) {
        chdir("$base/$program/build") or die "Cannot cd to $base/$program/build: $!\n";
        my @cmd = ("./$program", "-p", $standalone_data, @ARGV);
        print ">> @cmd\n";
        exec(@cmd) or die "Failed to exec $program: $!\n";
    }
}

die "Error: Could not find executable for '$program'\n";
