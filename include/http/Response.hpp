#pragma once
#include <string>
class HttpResponse {
    //determine correct content-type (text/html,text/css,etc.)
    static std::string get_mime_type(const std::string &filepath);
public:
    //static file to the client socket using zero-copy sendfile()
    static void send_file(int client_fd, const std::string &filepath);
    //404 not found
    static void send_404(int client_fd);
};