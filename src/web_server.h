#include <string>

class WebServer {
public:
    WebServer(std::string port);
    void run();

private:
    int socket_fd;
};
