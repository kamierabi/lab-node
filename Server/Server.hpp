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

#ifdef _WIN32
    #define CLOSE_SOCKET closesocket
    #define MODULE_HANDLE HMODULE
    #define LOAD_MODULE(path) LoadLibrary(path)
    #define FREE_MODULE FreeLibrary(handle)
    #define GET_SYMBOL(handle, symbol) GetProcAddress(handle, symbol)
    #define MODULE_ERROR_t DWORD
    #define GET_ERROR() GetLastError()
#else
    #define CLOSE_SOCKET close
    #define MODULE_HANDLE void*
    #define LOAD_MODULE(path) dlopen(path, RTLD_NOW)
    #define FREE_MODULE(handle) dlclose(handle)
    #define GET_SYMBOL(handle, symbol) dlsym(handle, symbol)
    #define MODULE_ERROR_t const char*
    #define GET_ERROR() dlerror()
#endif


struct call_signature {
    uint8_t* buffer_in;       // pointer to input buffer
    size_t buf_in_size;       // size of input buffer
    uint8_t* buffer_out;      // pointer to output buffer
    size_t buf_out_size;      // size of output buffer expected to the client
    uint8_t* buffer_err;      // pointer to error buffer
    size_t* data_written;     // pointer to the size of actually written data to shrink output buffer
};

struct Module {
    MODULE_HANDLE handle;                           ///< Указатель на загруженную библиотеку
    std::vector<std::string> functions;             ///< Список функций, экспортируемых библиотекой
    std::string lib_name;                           ///< Имя библиотеки
};

namespace fs = std::filesystem;
using func_type = void(call_signature*);
using lab_vtable = std::vector<std::vector<std::function<void(call_signature*)>>>;
constexpr int MAX_BUFFER_SIZE = MAX_PACKET_SIZE;


class tcp_server {
public:
    tcp_server(int port);
    ~tcp_server();

    std::vector<Module> modules;
    // std::shared_ptr<lab_vtable> modules_vtable;
    lab_vtable modules_vtable;

    void start();
    bool check_error(std::vector<uint8_t>& vec_err);
private:
    int port;
    int server_socket;
    std::vector<std::thread> threads;
    std::mutex thread_mutex;
    Logger logger;

    void load_manifest();
    void populate_vtable();
    void init_server();
    void handle_connection(int client_socket);

    LAB_STATUS module_execute(
        uint8_t module_id,
        uint8_t operation_id, 
        std::vector<uint8_t>& buffer_in, 
        std::vector<uint8_t>& buffer_out, 
        std::vector<uint8_t>& buffer_err, 
        size_t& data_size
    );
};
