#include <cstdio>
#include <iostream>
#include <memory.h>
#include <set>

#include "socket_os.h"
#include "utils.h"
#include "web_server.h"

const int BACKLOG = 10;

std::set<std::string> routes = {"/", "/index.html", "/style.css",
                                "/script_password_chage.js"};

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

int WebServer::acceptClientConnection() {
    sockaddr_storage client_addr;
    socklen_t sin_size = sizeof client_addr;

    int client_fd = accept(socket_fd, (sockaddr *)&client_addr, &sin_size);
    if (client_fd == -1) {
        perror("accept");
        std::cout << socket_fd << std::endl;
        return -1;
    }

    char client_addr_string[INET6_ADDRSTRLEN];
    inet_ntop(client_addr.ss_family, &((sockaddr_in *)&client_addr)->sin_addr,
              client_addr_string, sizeof client_addr_string);

    std::cout << client_addr_string << " connected" << std::endl;
    return client_fd;
}

void WebServer::run() {
    while (1) {
        auto client_fd = acceptClientConnection();

        char buffer[1024] = {};
        read(client_fd, buffer, 1024);
        std::cout << buffer << std::endl << std::endl;

        auto request = std::string(buffer);

        auto route_opt = get_route(request);
        if (!route_opt) {
            send_error_and_close(client_fd, ErrorStatus::BAD_REQUEST);
            continue;
        }

        auto route = *route_opt;
        std::cout << "Route: \"" << route << "\"" << std::endl;
        if (!routes.count(route)) {
            send_error_and_close(client_fd, ErrorStatus::NOT_FOUND);
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
