#include "Server.hpp"


tcp_server::tcp_server(int _port) : 
    loader(std::string("./modules")),
    port(_port),
    server_socket(-1),
    logger()
{}

tcp_server::~tcp_server() {
    CLOSE_SOCKET(server_socket);
#ifdef WIN32
    WSACleanup();
#endif
}


bool tcp_server::check_error(std::vector<uint8_t>& vec_err) {
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
    int opt = 1;

    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        throw std::runtime_error("Failed to set socket option REUSEADDR");
    }

    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt))) {
        throw std::runtime_error("Failed to set socket option REUSEADDR");
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
    init_server();
    std::cout << "Server listening on port " << port << std::endl;
    logger.log(Logger::LogLevel::DEBUG, "Server is listening on port " + std::to_string(port));

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
    auto start = std::chrono::high_resolution_clock::now();
    // Buffer for receiving data from the client
    
    std::vector<uint8_t> recv_buffer(MAX_PACKET_SIZE);

    // Receive data from the client
    ssize_t received_bytes = recv(client_socket, recv_buffer.data(), MAX_PACKET_SIZE, 0);
    if (received_bytes < PROTOCOL_V1_HEADER_SIZE) {
        std::cerr << "Error: Insufficient data received from client." << std::endl;
        logger.log(Logger::LogLevel::ERROR, "Insufficient data received from client");
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
    int exec_result = loader.execute(
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
            logger.log(
            Logger::LogLevel::ERROR,
            std::string( 
                "Error occured in " + loader.modules[module_id-1].get_module_name() + ':' + loader.modules[module_id-1].get_function_name(function_id) + ' ' + err 
            ));
        }

    }
    CLOSE_SOCKET(client_socket);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::micro> elapsed = end - start;
    logger.log(Logger::LogLevel::DEBUG, std::string("Request took " + std::to_string(elapsed.count()) + " µs"));
}

void tcp_server::print_uint8_vector(const std::vector<uint8_t>& vec) {
    for (uint8_t byte : vec) {
        std::cout << static_cast<int>(byte) << ' ';
    }
    std::cout << '\n';
}
