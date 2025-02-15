#include "./Server/Server.hpp"

constexpr int DEFAULT_PORT = 9481;
// TODO: не плодить логи при failed to bind socket
// TODO: обработка SIGSEGV который может возникнуть при вызове библиотеки: нужно сохранить дамп пакета и backtrace в логе и вызвать std::exception
// TODO: разделить репозитории с модулями и с самим сервером
// TODO: протестировать совместимость с Windows 
// TODO: разделить репозитории с модулями и с самим сервером 
// TODO: написать makefile для отдельной компиляции сервера, отдельной компиляции одного модуля, всех модулей и всего проекта целиком
// TODO: README, LICENSE, копирайты на файлы


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
