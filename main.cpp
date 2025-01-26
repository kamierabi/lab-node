#include "./Server/Server.hpp"

constexpr int DEFAULT_PORT = 9481;
// TODO: починить формирование файла логов - 24 часовая система вместо 12 часовой
// TODO: логирование ошибок загрузки .so/.dll библиотек


int main() {
    try {
        tcp_server server(DEFAULT_PORT);
        server.start();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
