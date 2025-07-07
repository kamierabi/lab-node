#pragma once
#include "../Logger/Logger.hpp"
#include "../Loader/Loader.hpp"
#include "../ThreadPool/ThreadPool.hpp"
#include <cstring>
#include <iostream>
#include <csignal>

#ifdef _WIN32
    #include <windows.h>
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <psapi.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <dlfcn.h>
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <unistd.h>
    #include <sys/resource.h>
#endif

#define PROTOCOL_V1_HEADER_SIZE 4
#define MAX_PACKET_SIZE                 4096
#define BUFFER_OUTPUT_SIZE              4096
#define BUFFER_ERROR_SIZE               4096
#define PROTOCOL_V1                     0x01
#define SUCCESS                         0x00

#ifdef _WIN32
    #define CLOSE_SOCKET closesocket
#else
    #define CLOSE_SOCKET close
#endif

constexpr int MAX_BUFFER_SIZE = MAX_PACKET_SIZE;

/**
 * @brief Многопоточный TCP-сервер
 */
class Server {
public:
    Server(int port, size_t thread_count);
    ~Server();

    void run();
    bool check_error(std::vector<uint8_t>& vec_err);
    void print_uint8_vector(const std::vector<uint8_t>& vec);

private:
    void accept_loop();
    void handle_client(int client_socket);

    int port_;
    int server_socket_;
    Logger logger_;
    Loader loader_;
    ThreadPool thread_pool_;
};
