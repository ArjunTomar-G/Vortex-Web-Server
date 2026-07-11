#include "http/Response.hpp"
#include "utils/Logger.hpp"
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <unistd.h>

std::string HttpResponse::get_mime_type(const std::string& filepath) {
    if(filepath.rfind(".html")!= std::string::npos) return "text/html";
    if(filepath.rfind(".css") != std::string::npos) return "text/css";
    if(filepath.rfind(".js")  != std::string::npos) return "application/javascript";
    if(filepath.rfind(".png") != std::string::npos) return "image/png";
    if(filepath.rfind(".jpg") != std::string::npos) return "image/jpeg";
    return "text/plain";
}
void HttpResponse::send_file(int client_fd, const std::string& filepath) {
    //open the file on disk in read-only mode
    int file_fd = open(filepath.c_str(), O_RDONLY);
    // file doesn't exist == 404
    if (file_fd== -1){
        send_404(client_fd);
        return;
    }
    //fstat for file size (in bytes)
    struct stat file_stat;
    if (fstat(file_fd, &file_stat) == -1){
        close(file_fd);
        send_404(client_fd);
        return;
    }
    off_t file_size = file_stat.st_size;
    //standard HTTP/1.1 response header (just google it!)
    std::string mime_type = get_mime_type(filepath);
    std::string headers = "HTTP/1.1 200 OK\r\n"
                          "Server: Vortex\r\n"
                          "Content-Type: " + mime_type + "\r\n"
                          "Content-Length: " + std::to_string(file_size) + "\r\n"
                          "Connection: close\r\n\r\n";
                          //closing connection per request for simplicity
    //write text headers to the client socket
    write(client_fd, headers.c_str(), headers.length());

    //** stream the file directly from disk cache to network socket
    off_t offset = 0;
    size_t bytes_remaining = file_size;
    /* sockets can be non-blocking...sendfile might send only part of the file 
    if the kernel's network buffer fills up...loop until it's completely sent */
    while (bytes_remaining > 0) {
        ssize_t sent_bytes = sendfile(client_fd, file_fd, &offset, bytes_remaining);
        if (sent_bytes <0) {
            //error handling
            if (errno == EAGAIN || errno == EWOULDBLOCK){
                // network buffer is temporarily full...spin or sleep until the buffer clears up.
                usleep(1000); 
                continue;
            }
            //socket error occurred (like client disconnected midway)
            break;
        }
        //unexpected file end
        if (sent_bytes ==0) break;
        bytes_remaining -= sent_bytes;
    }

    //clean up...file descriptor
    close(file_fd);
}

void HttpResponse::send_404(int client_fd) {
    std::string body = "<html><body><h1>404 Not Found</h1><p>Vortex Server could not find the file.</p></body></html>";
    std::string response = "HTTP/1.1 404 Not Found\r\n"
                           "Server: Vortex\r\n"
                           "Content-Type: text/html\r\n"
                           "Content-Length: " + std::to_string(body.length()) + "\r\n"
                           "Connection: close\r\n\r\n" + body;
    write(client_fd, response.c_str(), response.length());
}