#!/usr/bin/env perl 

use strict;
use warnings;
use LWP::UserAgent;
use Encode qw(encode decode);

my $rank_url = 'http://openapi.naver.com/search?key=b7d595844816c0b937e204837a33b432&query=nexearch&target=rank';

my $ua = LWP::UserAgent->new;
my $resp = $ua->get($rank_url);
my $decode_body;

if ($resp->is_success) {
     $decode_body = $resp->decoded_content;
}
else {
    die $resp->status_line;
}

my @top_tens = $decode_body =~ m{<K>(.*?)</K>}gsm; 
my $file = "top_ten.log";

open(my $fh, ">", $file) or die "cannot open > top_ten.log: $!"; 

foreach my $p ( @top_tens ) {
    print $fh "$p\n";
}
close $fh;
