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

my $fp = '/tmp/top_ten.log';
if ( -f $fp ) { system("rm $fp");}

foreach my $p ( @top_tens ) {
    print "$p\n";
    system("echo $p > /tmp/top_ten.log");
}
