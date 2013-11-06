#!/usr/bin/env perl 

use strict;
use warnings;
use LWP::UserAgent;
use Encode qw(encode decode);

my $rank_url = 'http://openapi.naver.com/search?key=b7d595844816c0b937e204837a33b432&query=nexearch&target=rank';

my $ua = LWP::UserAgent->new;
my $resp = $ua->get($rank_url);

if ($resp->is_success) {
    print $resp->decoded_content;
}
else {
    die $resp->status_line;
}
