#include "./Server/Server.hpp"

#include <iostream>
#include <cstdlib>
#include <getopt.h>

int port = 9481;
size_t thread_count = 8;

int main(int argc, char* argv[]) {
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
                std::cerr << "Usage: " << argv[0] << " [-p port] [-t thread_count]" << std::endl;;
                return 1;
        }
    }

    try {
        Server server(port, thread_count);
        std::cout << "Server is running on port " << port << std::endl;
        server.run();
    } catch (const std::exception& e) {
        std::cerr << (std::string("Fatal error: ") + e.what()) << std::endl;
        return 1;
    }

    return 0;
}