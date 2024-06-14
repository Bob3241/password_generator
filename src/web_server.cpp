#include "./web_server.h"
#include "./socket_os.h"
#include <cstdio>
#include <iostream>
#include <memory.h>
#include <sys/socket.h>
#include <fstream>
#include <filesystem>
#include <set>

const int BACKLOG = 10;

void send_file(int client_fd, std::string route);
std::string get_route(std::string request);

std::set<std::string> routes = {"/", "/index.html", "/style.css" ,"/script_password_chage.js"};

WebServer::WebServer(std::string port) {
    addrinfo hints = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    addrinfo *server_info;

    int rv = getaddrinfo(nullptr, port.c_str(), &hints, &server_info);
    if (rv) {
        std::cout << "getaddrinfo error: " << gai_strerror(rv) << std::endl;
    }

    for (auto p = server_info;; p = p->ai_next) {
        if (p == nullptr) {
            std::cout << "server failed to bind" << std::endl;
            exit(1);
        }

        socket_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

        if (socket_fd == -1) {
            perror("server: socket");
        }

        int yes = 1;
        if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes,
                       sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(socket_fd, p->ai_addr, p->ai_addrlen) == -1) {
            close(socket_fd);
            perror("server: bind");
            continue;
        }
        break;
    }

    freeaddrinfo(server_info);

    if (listen(socket_fd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    std::cout << "Server is running on http://localhost:" << port << std::endl;
}

void WebServer::run() {
    while (1) {
        sockaddr_storage client_addr;
        socklen_t sin_size = sizeof client_addr;

        int client_fd = accept(socket_fd, (sockaddr *)&client_addr, &sin_size);
        if (client_fd == -1) {
            perror("accept");
            std::cout << socket_fd << std::endl;
            continue;
        }

        char client_addr_string[INET6_ADDRSTRLEN];
        inet_ntop(client_addr.ss_family,
                  &((sockaddr_in *)&client_addr)->sin_addr, client_addr_string,
                  sizeof client_addr_string);

        std::cout << client_addr_string << " connected" << std::endl;
        char buffer[1024] = {};
        read(client_fd, buffer, 1024);
        std::cout << buffer << std::endl << std::endl;

        auto request = std::string(buffer);

        auto route = get_route(request);
        if (route == "") {
            const auto answer = "HTTP/1.1 400 Bad Request\n\n";
            send(client_fd, answer, strlen(answer), 0);
            close(client_fd);
            continue;
        }

        std::cout << "Route: \"" << route << "\"" << std::endl;

        if (!routes.count(route)) {
            const auto answer = "HTTP/1.1 404 Not Found\n\n";
            close(client_fd);
            send(client_fd, answer, strlen(answer), 0);
            continue;
        }

        if (route == "/") {
            send_file(client_fd, "/index.html");
        } else {
            send_file(client_fd, route);
        }

        close(client_fd);
    }
}

void send_file(int client_fd, std::string route) {
    auto filename = "../public" + route;

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
    auto content_length_header = "Content-Length: " + std::to_string(file_size) + "\n\n";
    send(client_fd, content_length_header.c_str(), content_length_header.size(), 0);

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

std::string get_route(std::string request) {
        auto route_start = request.find(" ") + 1;
        auto route_end = request.find("HTTP");
        if (route_end == std::string::npos) {
            return "";
        }

        return request.substr(route_start, route_end - route_start - 1);
}
