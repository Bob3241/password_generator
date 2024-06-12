#include "./web_server.h"

int main() {
    WebServer server("8001");
    server.run();
}
