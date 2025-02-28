#include "./Server/Server.hpp"

constexpr int DEFAULT_PORT = 9481;
// TODO: не плодить логи при failed to bind socket
// TODO: обработка SIGSEGV который может возникнуть при вызове библиотеки: вызвать std::exception
// TODO: разделить репозитории с модулями и с самим сервером
// TODO: протестировать совместимость с Windows 
// TODO: README, LICENSE, копирайты на файлы


int main(int argc, char* argv[]) {
    try {
        if (argc < 2) {
            tcp_server server(DEFAULT_PORT);
            server.start();
        }
        else {
            tcp_server server(std::atoi(argv[1]));
            server.start();
        } 
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
