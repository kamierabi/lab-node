#include "Server.hpp"


// tcp_server::tcp_server(int port) : port(port), server_socket(-1) {}

tcp_server::tcp_server(int _port) {
    port = _port;
    server_socket = -1;
    Logger logger;
}

tcp_server::~tcp_server() {
    CLOSE_SOCKET(server_socket);
    for (auto& [transport_id, handle] : transport_map) {
#ifdef _WIN32
        FreeLibrary(handle);
#else
        dlclose(handle);
#endif
    }
#ifdef _WIN32
    WSACleanup();
#endif
}

void tcp_server::init_transports() {
    const fs::path transport_dir = "./transport";
    const fs::path manifest_file = transport_dir / "manifest.conf";

    if (!fs::exists(transport_dir) || !fs::is_directory(transport_dir)) {
        std::cerr << "No transport directory found." << std::endl;
        logger.log(Logger::LogLevel::ERROR, "No transport directory found.");
        return;
    }

    if (!fs::exists(manifest_file)) {
        std::cerr << "Manifest file not found in transport directory." << std::endl;
        logger.log(Logger::LogLevel::ERROR, "Manifest file not found in transport directory.");
        return;
    }

    // Parse the manifest file
    std::unordered_map<std::string, int> manifest_map;
    std::ifstream manifest(manifest_file);
    std::string line;

    while (std::getline(manifest, line)) {
        std::istringstream line_stream(line);
        std::string library_name;
        int transport_id;
        if (std::getline(line_stream, library_name, '=') && (line_stream >> transport_id)) {
            library_name = fs::path(library_name).filename().string();
            manifest_map[library_name] = transport_id;
        }
    }

    for (const auto& entry : fs::directory_iterator(transport_dir)) {
        if (entry.is_regular_file()) {
            const auto& path = entry.path();
            std::string extension = path.extension().string();
#ifdef _WIN32
            if (extension == ".dll") {
                HMODULE lib = LoadLibrary(path.string().c_str());
                if (lib) {
                    std::string filename = path.filename().string();
                    if (manifest_map.find(filename) != manifest_map.end()) {
                        transport_map[manifest_map[filename]] = lib;

                    } else {
                        FreeLibrary(lib);
                    }
                else {
                    FreeLibrary(lib);
                }
                }
            }
#else
            if (extension == ".so") {
                void* lib = dlopen(path.string().c_str(), RTLD_NOW);
                if (lib) {
                    std::string filename = path.filename().string();
                    if (manifest_map.find(filename) != manifest_map.end()) {
                        transport_map[manifest_map[filename]] = lib;
                    }
                    else {
                       dlclose(lib);  
                    } 
                }
                else {
                    dlclose(lib);
                }
            }
#endif
        }
    }
}

LAB_STATUS tcp_server::_execute(uint8_t transport_id, uint8_t _opcode, std::vector<uint8_t>& buffer_in, std::vector<uint8_t>& buffer_out, std::vector<uint8_t>& buffer_err) {
    int transport = static_cast<int>(transport_id);
    // int opcode = static_cast<int>(_opcode);
    if (transport_map.find(transport) != transport_map.end()) {
        switch(_opcode) {
            case OP_READ:
            {
                #ifdef _WIN32
                func_type* func_ptr = reinterpret_cast<func_type*>(GetProcAddress(transport_map[transport], "read"));
                DWORD getprocaddr_error = GetLastError();
                if (getprocaddr_error) {
                    std:cerr << "Failed to load function": << std::string(getprocaddr_error) << std::endl;
                    return ERROR_NO_OPERATION;
                }
                #else
                func_type* func_ptr = reinterpret_cast<func_type*>(dlsym(transport_map[transport], "read"));
                const char* dlsym_error = dlerror();
                if (dlsym_error) {
                    std::cerr << "Failed to load function: " << dlsym_error << std::endl;
                    return ERROR_NO_OPERATION;
                }
                #endif
                std::function<void(uint8_t*, uint8_t*, uint8_t*)> func = func_ptr;
                func(buffer_in.data(), buffer_out.data(), buffer_err.data());
                if (buffer_err.data() == NULL) {
                    return ERROR_MODULE_INTERNAL;
                }
                else {
                    return SUCCESS;
                }
            }
            case OP_WRITE:
            {
                #ifdef _WIN32
                func_type* func_ptr = reinterpret_cast<func_type*>(GetProcAddress(transport_map[transport], "write"));
                DWORD getprocaddr_error = GetLastError();
                if (getprocaddr_error) {
                    std:cerr << "Failed to load function": << std::string(getprocaddr_error) << std::endl;
                    return ERROR_NO_OPERATION;
                }
                #else
                func_type* func_ptr = reinterpret_cast<func_type*>(dlsym(transport_map[transport], "write"));
                const char* dlsym_error = dlerror();
                if (dlsym_error) {
                    std::cerr << "Failed to load function: " << dlsym_error << std::endl;
                    return ERROR_NO_OPERATION;
                }
                #endif
                std::function<void(uint8_t*, uint8_t*, uint8_t*)> func = func_ptr;
                func(buffer_in.data(), buffer_out.data(), buffer_err.data());
                if (buffer_err.data() == NULL) {
                    return ERROR_MODULE_INTERNAL;
                }
                else {
                    return SUCCESS;
                }
            }
            default: return ERROR_INVALID_OPERATION;
        }
    }
    else {
        return ERROR_INVALID_TRANSPORT;
    }
}

void tcp_server::init_server() {
#ifdef _WIN32
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        throw std::runtime_error("WSAStartup failed");
    }
#endif
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_socket, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
        throw std::runtime_error("Failed to bind socket");
    }

    if (listen(server_socket, SOMAXCONN) < 0) {
        throw std::runtime_error("Failed to listen on socket");
    }
}

void tcp_server::start() {
    init_transports();
    init_server();
    std::cout << "Server listening on port " << port << std::endl;
    logger.log(Logger::LogLevel::DEBUG, std::format("Server listening on port {}", port));

    while (true) {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_socket = accept(server_socket, reinterpret_cast<struct sockaddr*>(&client_addr), &client_len);

        if (client_socket < 0) {
            std::cerr << "Failed to accept client connection" << std::endl;
            logger.log(Logger::LogLevel::ERROR, "Failed to accept client connection");
            continue;
        }

        std::lock_guard<std::mutex> lock(thread_mutex);
        threads.emplace_back(&tcp_server::handle_connection, this, client_socket);
    }
}

void tcp_server::handle_connection(int client_socket) {
    // Buffer for receiving data from the client
    constexpr size_t recv_buffer_size = BUFFER_OUTPUT_SIZE;
    std::vector<uint8_t> recv_buffer(recv_buffer_size);

    // Receive data from the client
    ssize_t received_bytes = recv(client_socket, recv_buffer.data(), recv_buffer_size, 0);
    if (received_bytes < 4) {
        std::cerr << "Error: Insufficient data received from client." << std::endl;
        logger.log(Logger::LogLevel::ERROR, "Insufficient data received from client");
        close(client_socket);
        return;
    }

    // Allocate and populate the input buffer
    std::vector<uint8_t> buf_in(recv_buffer.begin() + 4, recv_buffer.begin() + received_bytes);

    // Allocate the output buffer
    size_t buf_out_size = (static_cast<size_t>(recv_buffer[2]) + 1) * BUFFER_OUTPUT_SIZE;
    std::vector<uint8_t> buf_out(buf_out_size);

    // Allocate the error buffer
    // constexpr size_t buf_err_size = BUFFER_ERROR_SIZE;
    std::vector<uint8_t> buf_err(BUFFER_ERROR_SIZE);

    // Execute the processing function
    LAB_STATUS exec_result = _execute(recv_buffer[1], recv_buffer[2], buf_in, buf_out, buf_err);
    uint8_t response_header[4] = {recv_buffer[0], recv_buffer[1], recv_buffer[2], exec_result};
    // Prepare response data
    if (exec_result == SUCCESS) {
        // Success: Send header and output buffer to the client
        std::vector<uint8_t> response_data;
        response_data.reserve(sizeof(response_header) + buf_out.size());
        response_data.insert(response_data.end(), response_header, response_header + sizeof(response_header));
        response_data.insert(response_data.end(), buf_out.begin(), buf_out.end());

        send(client_socket, response_data.data(), response_data.size(), 0);
    } else {
        // Failure: Print error buffer to standard output
        // std::cerr << "Error buffer contents:" << std::endl;
        // for (size_t i = 0; i < buf_err.size(); ++i) {
        //     std::cerr << std::hex << static_cast<int>(buf_err[i]) << " ";
        //     if ((i + 1) % 16 == 0) std::cerr << std::endl; // Line break for readability
        // }
        // std::cerr << std::endl;
         // Send only the header with the execution result to the client
        send(client_socket, response_header, sizeof(response_header), 0);

    // Close the socket
    }
    CLOSE_SOCKET(client_socket);
}
