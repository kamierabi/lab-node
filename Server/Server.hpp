#pragma once
#include "lab_protocol.h"
#include "../Logger/Logger.hpp"
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
#include <csignal>


#ifdef _WIN32
    #include <windows.h>
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <dlfcn.h>
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif

namespace fs = std::filesystem;
using func_type = void(uint8_t*, uint8_t*, uint8_t*);
constexpr int MAX_BUFFER_SIZE = MAX_PACKET_SIZE;

#ifdef _WIN32
#define CLOSE_SOCKET closesocket
#else
#define CLOSE_SOCKET close
#endif

/**
 * @class tcp_server
 * @brief A TCP echo server class for handling client connections.
 */

class tcp_server {
public:
    tcp_server(int port);
    ~tcp_server();
    #ifdef _WIN32
    std::unordered_map<int, HMODULE> transport_map;
    #else
    std::unordered_map<int, void*> transport_map;
    #endif

    /**
     * @brief Starts the server.
     */
    void start();
private:
    int port;
    int server_socket;
    std::vector<std::thread> threads;
    std::mutex thread_mutex;
    Logger logger;

    /**
     * @brief Initializes the transport handler.
     */
    void init_transports();

    /**
     * @brief Initializes the server socket.
     */
    void init_server();

    /**
     * @brief Handles individual client connections.
     * @param client_socket The socket descriptor for the client.
     */
    void handle_connection(int client_socket);
    LAB_STATUS _execute(uint8_t transport_id, uint8_t _opcode, std::vector<uint8_t>& buffer_in, std::vector<uint8_t>& buffer_out, std::vector<uint8_t>& buffer_err);
};
