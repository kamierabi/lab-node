#pragma once

#define MAX_PACKET_SIZE                 4096
#define BUFFER_OUTPUT_SIZE              4096
#define BUFFER_ERROR_SIZE               4096
#define PROTOCOL_V1                     0x01

#define SUCCESS                         0x00
#define ERROR_INVALID_FORMAT            0x01
#define ERROR_INVALID_PROTOCOL          0x02
#define ERROR_INVALID_TRANSPORT         0x03
#define ERROR_INVALID_OPERATION         0x04
#define ERROR_MODULE_INTERNAL           0x05
#define ERROR_NO_OPERATION              0x06
#define NO_OP                           0xFF

#define PROTOCOL_V1_HEADER_SIZE 4

#include "../Logger/Logger.hpp"
#include "../Loader/Loader.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <cstring>
#include <mutex>
#include <stdexcept>
#include <string>
#include <cstring>
#include <string>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <functional>
#include <algorithm>


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
    #include <unistd.h>
    #include <sys/resource.h>
#endif

#ifdef _WIN32
    #define CLOSE_SOCKET closesocket
#else
    #define CLOSE_SOCKET close
#endif


constexpr int MAX_BUFFER_SIZE = MAX_PACKET_SIZE;


class tcp_server {
public:
    tcp_server(int port);
    ~tcp_server();

    Loader loader;
    void start();
    bool check_error(std::vector<uint8_t>& vec_err);
private:
    int port;
    int server_socket;
    std::vector<std::thread> threads;
    std::mutex thread_mutex;
    Logger logger;

    void init_server();
    void handle_connection(int client_socket);
    void print_uint8_vector(const std::vector<uint8_t>& vec);

};
