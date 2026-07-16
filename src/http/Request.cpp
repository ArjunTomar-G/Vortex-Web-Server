#include "http/Request.hpp"
#include "utils/Logger.hpp"
#include <sstream>
#include <iostream>

void HttpRequest::parse(const std::string& raw_request) {
    std::istringstream stream(raw_request);
    
    // The first line of an HTTP request looks like: GET /index.html HTTP/1.1
    stream >> method >> uri >> version;

    // Default route: if they just ask for "/", give them the homepage
    if (uri == "/") {
        uri = "/index.html";
    }

    // Map the requested URI to your local "public" folder
    filepath = "public" + uri;
    
    Logger::info("[PARSER] Method: " + method + " | Requested File: " + filepath);
}

std::string HttpRequest::get_method() const { return method; }
std::string HttpRequest::get_uri() const { return uri; }
std::string HttpRequest::get_filepath() const { return filepath; }