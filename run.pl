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

if (!$program) {
    print "Usage: ./run <program_name> [extra args...]\n\n";
    print "Available programs:\n";

    my %progs;
    if (-d $build_dir) {
        opendir(my $dh, $build_dir);
        while (my $entry = readdir($dh)) {
            next if $entry =~ /^\./;
            $progs{$entry} = 1 if -d "$build_dir/$entry";
        }
        closedir($dh);
    }
    
    for my $p (sort keys %progs) {
        print "  $p\n";
    }
    exit 1;
}

my $data_path = "$source_dir/$program";
my $exe_path = "$build_dir/$program/$program";

if (!-x $exe_path) {
    for my $base ($root, $parent) {
        my $alt_exe = "$base/$program/build/$program";
        if (-x $alt_exe) {
            $exe_path = $alt_exe;
            $data_path = "$base/$program"; 
            last;
        }
    }
}

if (-x $exe_path) {
    my $exe_dir = dirname($exe_path);
    my $exe_name = basename($exe_path);

    chdir($exe_dir) or die "Cannot cd to $exe_dir: $!\n";

    if (!-d $data_path) {
        warn "Warning: Data directory '$data_path' not found.\n";
    }

    my @cmd = ("./$exe_name", "-p", $data_path, @ARGV);
    
    print ">> Executing: @cmd\n";
    exec(@cmd) or die "Failed to exec $exe_name: $!\n";
} else {
    die "Error: Could not find executable for '$program' at $exe_path\n";
}
