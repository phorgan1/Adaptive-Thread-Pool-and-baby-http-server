#include "../http.h"
#include <iostream>
#include <assert.h>

int
main()
{
    size_t tests=0,passed=0,failed=0;
    std::cout << "test 1 - GET / HTTP/1.1";
    tests++;
    http_request_line h1("GET / HTTP/1.1");
    if(h1.to_string()!="GET / HTTP/1.1"){
	std::cout << "failed\n";
	failed++;
    }else if(!:q{
	std::cout << "passed\n";
	passed++;
    }
    tests++;
    http_request_line h2("FOO / HTTP/1.1");
    if(h1.to_string()!="GET / HTTP/1.1"){
	std::cout << "failed\n";
	failed++;
    }else{
	std::cout << "passed\n";
	passed++;
    }
    
    std::cout << tests << " tests, passed: " << passed << ", failed: " << failed << '\n';
    return 0;
}

