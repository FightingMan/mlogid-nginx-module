# vi:filetype=

#repeat_each(3);

use lib 'lib';
use Test::Nginx::Socket;

plan tests => repeat_each() * 2 * blocks();

run_tests();

__DATA__

=== TEST 1: main test
--- config
    location /main {
        mlogid on;
        echo $mlogid;
    }
--- request
    GET /main
--- response_body_like
ubuntu.+
--- error_code: 200
