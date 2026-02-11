#!/usr/bin/env perl

use strict;
use warnings;
use File::Find;
use File::Basename;
use File::Path qw(make_path);

my $src_dir = $ARGV[0];
my $out_dir = $ARGV[1];

die "Usage: $0 <source_dir> <output_dir>\n" if !$src_dir or !$out_dir;

make_path($out_dir) unless -d $out_dir;

find({
    wanted => sub {
        my $path = $File::Find::name;

        if (-d $path) {
            my $dir_name = basename($path);
            return if $path eq $src_dir;
            return if $dir_name eq "." || $dir_name eq "..";

            if ($dir_name !~ /^vk_/) {
                $File::Find::prune = 1;
                return;
            }
        }

        if ($path =~ /\.(vert|frag)$/) {
            my @parts = split('/', $path);
            my $parent_dir = $parts[-2]; 
            my $filename = basename($path);
            
            my $safe_name = $filename;
            $safe_name =~ s/\./_/g;
            
            my $output_file = "$out_dir/${parent_dir}_$safe_name.spv";
            
            my @cmd = ("glslc", $path, "-o", $output_file);
            print join(" ", @cmd) . "\n";
            system(@cmd);
        }
    },
    no_chdir => 1,
}, $src_dir);
