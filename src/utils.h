#include <optional>
#include <string>

enum class ErrorStatus {
    NOT_FOUND,
    BAD_REQUEST,
};

void send_file(int client_fd, std::string route);
std::optional<std::string> get_route(std::string request);
void send_error_and_close(int client_fd ,ErrorStatus error);
