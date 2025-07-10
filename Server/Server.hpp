#pragma once
#include "../Logger/Logger.hpp"
#include "../Loader/Loader.hpp"
#include "../ThreadPool/ThreadPool.hpp"
#include <cstring>
#include <iostream>
#include <csignal>


#ifdef _WIN32
    #include <Windows.h>
    #define CLOSE_SOCKET(s) closesocket(s)
    typedef int socklen_t;
    typedef SSIZE_T ssize_t;
    #pragma comment(lib, "Ws2_32.lib")
#else
    #include <unistd.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <csignal>
    #define CLOSE_SOCKET(s) close(s)
#endif

#include <chrono>
#include <sstream>
#include <vector>
#include <cstring>

#define PROTOCOL_V1_HEADER_SIZE 4
#define MAX_PACKET_SIZE                 4096
#define BUFFER_OUTPUT_SIZE              4096
#define BUFFER_ERROR_SIZE               4096
#define PROTOCOL_V1                     0x01
#define SUCCESS                         0x00


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
