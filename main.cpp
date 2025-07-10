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

int main(int argc, char* argv[]) {
#ifndef _WIN32
    int opt;
    while ((opt = getopt(argc, argv, "p:t:")) != -1) {
        switch (opt) {
            case 'p':
                port = std::atoi(optarg);
                break;
            case 't':
                thread_count = std::atoi(optarg);
                break;
            default:
                std::cerr << "Usage: " << argv[0] << " [-p port] [-t thread_count]" << std::endl;
                return 1;
        }
    }
#else
    // Windows fallback: simple manual parsing
    for (int i = 1; i < argc - 1; ++i) {
        if (std::string(argv[i]) == "-p") {
            port = std::atoi(argv[++i]);
        } else if (std::string(argv[i]) == "-t") {
            thread_count = std::atoi(argv[++i]);
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
