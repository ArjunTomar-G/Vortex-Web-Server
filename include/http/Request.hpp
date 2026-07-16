#pragma once

#include <string>

class HttpRequest {
private:
    std::string method;
    std::string uri;
    std::string version;
    std::string filepath; // The actual file location on disk (e.g., public/index.html)

public:
    // Parse the raw incoming bytes
    void parse(const std::string& raw_request);
    
    // Getters
    std::string get_method() const;
    std::string get_uri() const;
    std::string get_filepath() const;
};