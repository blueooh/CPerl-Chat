#!/usr/bin/expect
set timeout 10

spawn ./client/cperl-chat
expect {
  "your id:" {
    exp_send "pio\r"
    exp_continue
    }
  "Server Name:" {
    exp_send "perl.blogock.com\r"
    }
  timeout {
    exit
    }
  eof {
    exit
    }
}
interact
