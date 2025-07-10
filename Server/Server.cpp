#include "Server.hpp"

#include <chrono>
#include <sstream>
#include <vector>
#include <cstring>

Server::Server(int port, size_t thread_count)
    : port_(port),
      server_socket_(-1),
      logger_(),
      loader_("modules"),
      thread_pool_(thread_count)
{
#ifndef _WIN32
    signal(SIGPIPE, SIG_IGN);
#else
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
    setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(static_cast<uint16_t>(port_));

    if (bind(server_socket_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
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
#ifdef _WIN32
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
        int client_socket = accept(server_socket_, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
        if (client_socket < 0) {
            logger_.log(Logger::LogLevel::ERR, "Accept failed");
            continue;
        }

        thread_pool_.enqueue([this, client_socket]() {
            handle_client(client_socket);
        });
    }
}

void Server::handle_client(int client_socket) {
    auto start = std::chrono::high_resolution_clock::now();

    std::vector<uint8_t> recv_buffer(MAX_PACKET_SIZE);

    ssize_t received_bytes = recv(client_socket, reinterpret_cast<char*>(recv_buffer.data()), MAX_PACKET_SIZE, 0);
    if (received_bytes < PROTOCOL_V1_HEADER_SIZE) {
        logger_.log(Logger::LogLevel::ERR, "Insufficient data received from client");
        CLOSE_SOCKET(client_socket);
        return;
    }

    std::vector<uint8_t> buf_in(recv_buffer.begin() + PROTOCOL_V1_HEADER_SIZE, recv_buffer.begin() + received_bytes);

    size_t buf_out_size = (static_cast<size_t>(recv_buffer[2]) + 1) * BUFFER_OUTPUT_SIZE;
    std::vector<uint8_t> buf_out(buf_out_size);

    std::vector<uint8_t> buf_err(BUFFER_ERROR_SIZE);

    size_t bytes_written = 0;
    size_t module_id = static_cast<size_t>(recv_buffer[1]);
    size_t function_id = static_cast<size_t>(recv_buffer[2]);
    int exec_result = loader_.execute(
        module_id,
        function_id,
        buf_in,
        buf_out,
        buf_err,
        bytes_written
    );

    uint8_t result_bytes = static_cast<uint8_t>(exec_result);
    uint8_t response_header[4] = {recv_buffer[0], recv_buffer[1], recv_buffer[2], result_bytes};

    if (exec_result == SUCCESS) {
        std::vector<uint8_t> response_data;
        buf_out.resize(bytes_written);
        response_data.reserve(sizeof(response_header) + buf_out.size());
        response_data.insert(response_data.end(), response_header, response_header + sizeof(response_header));
        response_data.insert(response_data.end(), buf_out.begin(), buf_out.end());
        send(client_socket, reinterpret_cast<const char*>(response_data.data()), static_cast<int>(response_data.size()), 0);
    } else {
        send(client_socket, reinterpret_cast<const char*>(response_header), sizeof(response_header), 0);
        if (check_error(buf_err)) {
            std::string err(reinterpret_cast<const char*>(buf_err.data()), buf_err.size());
            logger_.log(
                Logger::LogLevel::ERR,
                "Error occured in " + loader_.modules[module_id].get_module_name() + ':' + loader_.modules[module_id].get_function_name(function_id) + ' ' + err
            );
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
        } else if (found_non_zero) {
            vec_err.resize(new_size);
            break;
        }
    }
    return found_non_zero;
}
