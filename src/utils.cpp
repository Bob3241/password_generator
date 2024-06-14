#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory.h>
#include <sstream>
#include <string>

#include "./socket_os.h"
#include "./utils.h"

void send_file(int client_fd, std::string route) {
    auto filename = "./public" + route;

    std::ifstream file(filename, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        const auto answer = "HTTP/1.1 404 Not Found\n\n";
        send(client_fd, answer, strlen(answer), 0);

        std::cout << "File not found" << std::endl;
        return;
    }

    const auto answer = "HTTP/1.1 200 OK\n";
    send(client_fd, answer, strlen(answer), 0);

    auto file_size = std::filesystem::file_size(filename);
    printf("File size: %ld\n", file_size);
    auto content_length_header =
        "Content-Length: " + std::to_string(file_size) + "\n\n";
    send(client_fd, content_length_header.c_str(), content_length_header.size(),
         0);

    char buffer[1024] = {};
    while (file) {
        file.read(buffer, 1024);
        auto bytes_sent = send(client_fd, buffer, file.gcount(), 0);
        if (bytes_sent == -1) {
            perror("send");
            break;
        }
        memset(buffer, 0, sizeof buffer);
    }
    file.close();
}

std::optional<std::string> get_route(std::string request) {
    std::string method, route;
    std::stringstream ss;

    ss << request;
    ss >> method >> route;

    if (method != "GET") {
        return {};
    }

    return route;
}

void send_error_and_close(int client_fd, ErrorStatus error) {
    std::string label;

    switch (error) {
    case ErrorStatus::BAD_REQUEST:
        label = "400 Bad Request";
        break;
    case ErrorStatus::NOT_FOUND:
        label = "404 Not Found";
        break;
    }

    auto answer = "HTTP/1.1 " + label + "\n\n";
    send(client_fd, answer.c_str(), answer.length(), 0);
}
