#include "http/Request.hpp"
#include "utils/Logger.hpp"
#include <iostream>
#include <cassert>
#include <string>

void test_standard_request() {
    std::cout << "[TEST 1] Parsing a standard file request...\n";
    
    // Simulate a raw string coming in from Google Chrome
    std::string raw_http = "GET /style.css HTTP/1.1\r\nHost: localhost:8080\r\n\r\n";
    
    HttpRequest req;
    req.parse(raw_http);

    // If any of these are wrong, the program will instantly crash (which is what we want in a test)
    assert(req.get_method() == "GET");
    assert(req.get_uri() == "/style.css");
    assert(req.get_filepath() == "public/style.css");
    
    std::cout << "   -> [SUCCESS] Method, URI, and Filepath parsed correctly.\n";
}

void test_root_routing() {
    std::cout << "\n[TEST 2] Parsing the root '/' homepage request...\n";
    
    // Simulate a user just typing "localhost:8080" with no specific file
    std::string raw_http = "GET / HTTP/1.1\r\nHost: localhost:8080\r\n\r\n";
    
    HttpRequest req;
    req.parse(raw_http);

    assert(req.get_method() == "GET");
    assert(req.get_uri() == "/index.html"); // Proves your default routing works
    assert(req.get_filepath() == "public/index.html");

    std::cout << "   -> [SUCCESS] Root '/' safely rerouted to index.html.\n";
}

int main() {
    // 1. Boot up a temporary log file just for this test suite
    Logger::init("test_parser.log");

    std::cout << "==========================================\n";
    std::cout << "     VORTEX ENGINE: PARSER TEST SUITE     \n";
    std::cout << "==========================================\n";

    // 2. Run the tests
    test_standard_request();
    test_root_routing();

    std::cout << "\n==========================================\n";
    std::cout << "   ALL PARSER TESTS PASSED FLAWLESSLY!    \n";
    std::cout << "==========================================\n";

    return 0;
}