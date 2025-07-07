#include "Server.hpp"

Server::Server(int port, size_t thread_count) :
    port_(port),
    server_socket_(-1),
    logger_("log"),
    loader_(std::string("./modules")),
    thread_pool_(thread_count)
    {
        signal(SIGPIPE, SIG_IGN);
#ifdef _WIN32
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        throw std::runtime_error("WSAStartup failed");
    }
#endif

        server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket_ == -1) {
            throw std::runtime_error("Socket creation failed");
        }

        int opt = 1;
        setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port_);

        if (bind(server_socket_, (struct sockaddr*)&address, sizeof(address)) < 0) {
            throw std::runtime_error("Bind failed");
        }

        if (listen(server_socket_, 128) < 0) {
            throw std::runtime_error("Listen failed");
        }

        logger_.log(Logger::LogLevel::DEBUG, "Server initialized on port " + std::to_string(port_));
    }

Server::~Server() {
    if (server_socket_ != -1) {
        CLOSE_SOCKET(server_socket_);
#ifdef WIN32
        WSACleanup();
#endif
    }
    logger_.log(Logger::LogLevel::DEBUG, "Server shut down");
}

void Server::run() {
    logger_.log(Logger::LogLevel::DEBUG, "Server started");
    accept_loop();
}

void Server::accept_loop() {
    while (true) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int client_socket = accept(server_socket_, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            logger_.log(Logger::LogLevel::ERROR, "Accept failed");
            continue;
        }

        thread_pool_.enqueue([this, client_socket]() {
            handle_client(client_socket);
        });
    }
}

void Server::handle_client(int client_socket) {
    auto start = std::chrono::high_resolution_clock::now();
    // Buffer for receiving data from the client
    
    std::vector<uint8_t> recv_buffer(MAX_PACKET_SIZE);

    // Receive data from the client
    ssize_t received_bytes = recv(client_socket, recv_buffer.data(), MAX_PACKET_SIZE, 0);
    if (received_bytes < PROTOCOL_V1_HEADER_SIZE) {
        // std::cerr << "Error: Insufficient data received from client." << std::endl;
        logger_.log(Logger::LogLevel::ERROR, "Insufficient data received from client");
        CLOSE_SOCKET(client_socket);
        return;
    }

    // Allocate and populate the input buffer
    std::vector<uint8_t> buf_in(recv_buffer.begin() + PROTOCOL_V1_HEADER_SIZE, recv_buffer.begin() + received_bytes);

    
    // Allocate the output buffer
    size_t buf_out_size = (static_cast<size_t>(recv_buffer[2]) + 1) * BUFFER_OUTPUT_SIZE;
    std::vector<uint8_t> buf_out(buf_out_size);

    // Allocate the error buffer
    std::vector<uint8_t> buf_err(BUFFER_ERROR_SIZE);

    // Execute the processing function
    size_t bytes_written = 0;
    size_t module_id = static_cast<size_t>(recv_buffer[1]);
    size_t function_id = static_cast<size_t>(recv_buffer[2]);
    int exec_result = loader_.execute(
        module_id,                              // module id
        function_id,                            // function id
        buf_in,                                 // input buffer
        buf_out,                                // output buffer
        buf_err,                                // error buffer
        bytes_written                           // bytes written
    );
    uint8_t result_bytes = static_cast<uint8_t>(exec_result);
    uint8_t response_header[4] = {recv_buffer[0], recv_buffer[1], recv_buffer[2], result_bytes};

    if (exec_result == SUCCESS) {
        // Success: Send header and output buffer to the client
        std::vector<uint8_t> response_data;
        buf_out.resize(bytes_written);
        response_data.reserve(sizeof(response_header) + buf_out.size());
        response_data.insert(response_data.end(), response_header, response_header + sizeof(response_header));
        response_data.insert(response_data.end(), buf_out.begin(), buf_out.end());
        send(client_socket, response_data.data(), response_data.size(), 0);
    } else {
        // Send only the header with the execution result to the client
        send(client_socket, response_header, sizeof(response_header), 0);
        if (check_error(buf_err)) 
        {   
            std::string err(reinterpret_cast<const char*>(buf_err.data()), buf_err.size());
            logger_.log(
            Logger::LogLevel::ERROR,
            std::string( 
                "Error occured in " + loader_.modules[module_id].get_module_name() + ':' + loader_.modules[module_id].get_function_name(function_id) + ' ' + err 
            ));
        }

    }
    CLOSE_SOCKET(client_socket);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::micro> elapsed = end - start;
    logger_.log(Logger::LogLevel::DEBUG, "Request took " + std::to_string(elapsed.count()) + " µs");
}

void Server::print_uint8_vector(const std::vector<uint8_t>& vec) {
    for (uint8_t byte : vec) {
        std::cout << static_cast<int>(byte) << ' ';
    }
    std::cout << '\n';
}

bool Server::check_error(std::vector<uint8_t>& vec_err) {
    bool found_non_zero = false;
    size_t new_size = 0;
    for (size_t i = 0; i < vec_err.size(); ++i) {
        if (vec_err[i] != 0x00) {
            found_non_zero = true; 
            ++new_size;
        }
        else if (found_non_zero) {
            vec_err.resize(new_size);
            break;
        }
    }
    return found_non_zero;
}
