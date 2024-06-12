#include "./web_server.h"
#include "./socket.h"
#include <cstdio>
#include <iostream>
#include <memory.h>
#include <sys/socket.h>

const int BACKLOG = 10;

WebServer::WebServer(std::string port) {
    addrinfo hints = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    addrinfo *server_info;

    std::cout << port.c_str() << std::endl;
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

    std::cout << "Server is running on port " << port << std::endl;
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
        std::cout << buffer << std::endl;
        auto answer = "HTTP/1.1 200 OK\n\nHello, world";
        send(client_fd, answer, strlen(answer), 0);
        close(client_fd);
    }
}
