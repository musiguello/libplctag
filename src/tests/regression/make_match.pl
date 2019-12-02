#!/usr/bin/perl

use strict;

my $hex = <<EOP;
6f 00 44 00 =1 =1 =1 =1 00 00
00 00 >2 >2 >2 >2 >2 >2 >2 >2
00 00 00 00 00 00 00 00 01 00
02 00 00 00 00 00 b2 00 34 00
5b 02 20 06 24 01 0a 05 00 00
00 00 >4 >4 >4 >4 >5 >5 3d f3
45 43 50 21 01 00 00 00 40 42
0f 00 a2 0f 00 42 40 42 0f 00
a2 0f 00 42 a3 03 01 04 20 02
24 01
EOP

my @parts = split(/ \n/,$hex);
my @match = ();

foreach $part (@parts) {
    if($part =~ m/[0-9a-f]{2}/) {
        push(@match,"0x$part");
    } else {
        my $len = length(@match);
        if(length(@match) > 0) {
            my $match_elems = join(@match, ', ');
            print ", MATCH($len),$match_elems";
            @match = ();
        }
    }
}