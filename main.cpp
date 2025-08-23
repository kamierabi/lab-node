#include "./Server/Server.hpp"

#include <iostream>
#include <cstdlib>
#include <string>

#ifdef _WIN32
    #include <Windows.h>
#else
    #include <getopt.h>
#endif

int port = 9481;
size_t thread_count = 8;

/**
 * @brief Выводит справку по использованию программы.
 *
 * @param prog_name Имя исполняемого файла (обычно argv[0]).
 */
void print_help(const std::string& prog_name) {
    std::cout << "Usage: " << prog_name << " [options]\n\n"
              << "Options:\n"
              << "  -p <port>         Set server port (default: 9481)\n"
              << "  -t <threads>      Set number of worker threads (default: 8)\n"
              << "  -h, --help        Show this help message and exit\n"
              << std::endl;
}

int main(int argc, char* argv[]) {
#ifndef _WIN32
    const char* short_opts = "p:t:h";
    const struct option long_opts[] = {
        {"help", no_argument, nullptr, 'h'},
        {nullptr, 0, nullptr, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, short_opts, long_opts, nullptr)) != -1) {
        switch (opt) {
            case 'p':
                port = std::atoi(optarg);
                break;
            case 't':
                thread_count = std::atoi(optarg);
                break;
            case 'h':
                print_help(argv[0]);
                return 0;
            default:
                print_help(argv[0]);
                return 1;
        }
    }
#else
    // Windows fallback: simple manual parsing
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-p" && i + 1 < argc) {
            port = std::atoi(argv[++i]);
        } else if (arg == "-t" && i + 1 < argc) {
            thread_count = std::atoi(argv[++i]);
        } else if (arg == "-h" || arg == "--help") {
            print_help(argv[0]);
            return 0;
        } else {
            print_help(argv[0]);
            return 1;
        }
    }
#endif

    try {
        Server server(port, thread_count);
        std::cout << "Server is running on port " << port << std::endl;
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
