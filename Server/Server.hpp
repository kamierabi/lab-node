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

#define LAB_STATUS uint8_t
#define PROTOCOL_V1_HEADER_SIZE 4

// #include "lab_protocol.h"
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

struct ProcessInfo {
    long vsize;
    long rss;
    long user_cpu_time;
    long system_cpu_time;

    std::string to_string() const {
        return "Virtual Memory Size: " + std::to_string(vsize / 1024) + " KB\n" +
               "Resident Set Size: " + std::to_string(rss / 1024) + " KB\n" +
               "User CPU Time: " + std::to_string(user_cpu_time) + " s\n" +
               "System CPU Time: " + std::to_string(system_cpu_time) + " s\n";
    }

    void to_uint8_vector(std::vector<uint8_t>& buffer) const {
        auto append = [&buffer](const void* data, size_t size) {
            const uint8_t* bytes = static_cast<const uint8_t*>(data);
            buffer.insert(buffer.end(), bytes, bytes + size);
        };

        append(&vsize, sizeof(vsize));
        append(&rss, sizeof(rss));
        append(&user_cpu_time, sizeof(user_cpu_time));
        append(&system_cpu_time, sizeof(system_cpu_time));
    }

    size_t size(void) {
        return sizeof(vsize) + sizeof(rss) + sizeof(user_cpu_time) + sizeof(system_cpu_time);
    }
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
