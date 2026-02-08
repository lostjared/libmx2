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
        push @progs, $entry if -x $exe;
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

if (-x $exe && -d $data) {
    chdir("$build_dir/$program") or die "Cannot cd to $build_dir/$program: $!\n";
    my @cmd = ("./$program", "-p", $data, @ARGV);
    print ">> @cmd\n";
    exec(@cmd) or die "Failed to exec $program: $!\n";
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
