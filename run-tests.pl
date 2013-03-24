#!/usr/bin/perl
use strict;
use warnings;
use File::Basename;

sub red {
    return "\x1B[31m" . $_[0] . "\x1b[0m"
}

sub green {
    return "\x1B[32m" . $_[0] . "\x1b[0m"
}

my @test_files = glob('./tests/auto/*.u');
my $padding = 0;

foreach my $test_file (@test_files) {
    chomp($test_file);
    my $size = length(basename($test_file));
    if ($size > $padding) {
        $padding = $size;
    }
}
$padding += 3;

my $flags = '--eliminate-tail-recursion --lib-dir lib';

for my $test_file (@test_files) {
    my $output;
    my $compiler_output = `./compiler $flags $test_file 2>&1`;
    
    printf("%-${padding}s", basename($test_file));

    if (length($compiler_output) > 0) {
        $output = red('Compilation Failed');
    } else {
        my $test_output = `./output`;
        chomp($test_output);
        
        if ($test_output eq 'PASS') {
            $output = green('PASS');
        } elsif ($test_output eq 'FAIL') {
            $output = red('FAIL');
        } elsif ($test_output eq '') {
            $output = red('No output');
        } else {
            $output = red('Malformed output: ' . $test_output);
        }
    }
    
    print($output . "\n");
}
