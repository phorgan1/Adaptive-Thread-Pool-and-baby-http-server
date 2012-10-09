#include "../http.h"
#include <iostream>
#include <assert.h>

int
main()
{
    size_t tests=0,passed=0,failed=0;
    std::cout << "test 1 - 3 argument constructor - ";
    tests++;
    authority a("patrick","dbp-consulting.com","80");
    if(a.to_string()!="patrick@dbp-consulting.com:80"){
	std::cout << "failed\n";
	failed++;
    }else{
	std::cout << "passed\n";
	passed++;
    }
    tests++;
    std::cout << "test 2 - 2 argument constructor - ";
    authority b("dbp-consulting.com","80");
    if(b.to_string()!="dbp-consulting.com:80"){
	std::cout << "failed\n";
	failed++;
    }else{
	std::cout << "passed\n";
	passed++;
    }
    tests++;
    std::cout << "test 3 - 1 argument constructor - ";
    authority c("patrick@dbp-consulting.com:80");
    if(c.to_string()!="patrick@dbp-consulting.com:80"){
	std::cout << "failed\n";
	std::cout << "Expected patrick@dbp-consulting.com:80 but got '" << c.to_string() << '\n';
	failed++;
    }else{
	std::cout << "passed\n";
	passed++;
    }
    std::cout << tests << " tests, passed: " << passed << ", failed: " << failed << '\n';

    return 0;
}

