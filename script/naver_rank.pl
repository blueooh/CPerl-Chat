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

my @html_body = $decode_body =~ m{<K>(.*?)</K>}gsm; 
my @top_tens;
my $i = 1;

foreach my $p (@html_body) {
    push @top_tens, "$i. $p";
    $i++;
}

my $top_ten = join('/', @top_tens);
print "$top_ten\n";

my $fp = '/tmp/info_pipe';

open(my $fh, ">", $fp) or die "cannot open > top_ten.log: $!"; 
print $fh "$top_ten\n";
close $fh;
