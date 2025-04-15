#include "Server.hpp"


tcp_server::tcp_server(int _port) {
    port = _port;
    server_socket = -1;
    Logger logger;
}

tcp_server::~tcp_server() {
    CLOSE_SOCKET(server_socket);
    for (size_t i=0; i<modules.size(); i++) {
        FREE_MODULE(modules[i].handle);
    }
#ifdef WIN32
    WSACleanup();
#endif
}

void tcp_server::load_manifest() {
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

    std::ifstream file(manifest_file);
    if (!file) {
        std::cerr << "ERROR: cannot open file " << manifest_file << "\n";
        logger.log(Logger::LogLevel::ERROR, "cannot open manifest file.");
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream line_stream(line);
        std::string lib_entry;
        if (!std::getline(line_stream, lib_entry, '=')) {
            continue;
        }
        
        std::string lib_name = lib_entry;
        lib_name.erase(lib_name.find_last_not_of(" ") + 1); 
        
        std::string details;
        if (!std::getline(line_stream, details)) {
            continue;
        }
        
        std::istringstream details_stream(details);
        int module_id;
        char comma;
        details_stream >> module_id >> comma;
        
        if (module_id < 1 || module_id > 255 || comma != ',') {
            std::cerr << "Ошибка парсинга строки: " << line << "\n";
            logger.log(Logger::LogLevel::ERROR, "Error while parsing line " + std::string(line));
            throw std::runtime_error("Parsing error, check log");
        }
        
        std::vector<std::string> functions;
        std::string function_name;
        while (std::getline(details_stream, function_name, ',')) {
            function_name.erase(0, function_name.find_first_not_of(" ")); // Убираем пробелы слева
            function_name.erase(function_name.find_last_not_of(" ") + 1); // Убираем пробелы справа
            functions.push_back(function_name);
        }
        
        std::string lib_path = "./transport/" + lib_name;
        MODULE_HANDLE handle;
        handle = LOAD_MODULE(lib_path.c_str());
        if (!handle) {
            std::cerr << "Module loading error: " << lib_path << "\n";
            logger.log(Logger::LogLevel::ERROR, "Module was not found in manifest file " + lib_path);
            throw std::runtime_error("Loading module error, check log");
        }
        else {
            modules.push_back(Module{handle, functions, lib_name});
        }
    }
    populate_vtable();
    return;
}

void tcp_server::populate_vtable() {
    modules_vtable.reserve(modules.size());
    for (size_t i = 0; i <= modules.size() - 1; ++i) {
        std::vector<std::function<void(call_signature*)>> temp_vec;
        for (size_t j = 0; j <= modules[i].functions.size() - 1; ++j) {
            func_type* func_ptr = reinterpret_cast<func_type*>(GET_SYMBOL(modules[i].handle, modules[i].functions[j].c_str()));
            MODULE_ERROR_t funcptr_err = GET_ERROR();
            if (funcptr_err) {
                logger.log(Logger::LogLevel::ERROR, "Failed to load function: " + std::string(funcptr_err));
                throw std::runtime_error("Failed to load function from module, check log");
            }
            else {
                std::function<void(call_signature*)> func = func_ptr;
                temp_vec.push_back(func);
            }
       }
       modules_vtable.push_back(temp_vec);
    }
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

LAB_STATUS tcp_server::module_execute(
    uint8_t module_id,
    uint8_t operation_id,
    std::vector<uint8_t>& buffer_in,
    std::vector<uint8_t>& buffer_out,
    std::vector<uint8_t>& buffer_err,
    size_t& data_size
)
{
    size_t module_id_t = static_cast<size_t>(module_id);
    size_t operation_id_t = static_cast<size_t>(operation_id);
    std::cout << module_id_t << std::endl;
    std::cout << operation_id_t << std::endl;
    if (module_id_t == 0) {
        switch (operation_id_t) {
            default: {
                std::memcpy(buffer_out.data(), buffer_in.data(), buffer_in.size());
                data_size = buffer_in.size();
                break;
            }
            case 1: {
                std::ostringstream oss;
                for (const auto& mod : modules) {
                    oss << mod.lib_name << "=";
                    for (size_t i = 0; i < mod.functions.size(); ++i) {
                        if (i > 0) oss << ",";
                        oss << mod.functions[i];
                    }
                    oss << "\n";
                }
                std::memcpy(buffer_out.data(), oss.str().c_str(), oss.str().size());
                data_size = oss.str().size();
                break;
            }
            case 2: {
                ProcessInfo info;
#ifdef _WIN32
                PROCESS_MEMORY_COUNTERS pmc;
                GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
                info.vsize = pmc.PagefileUsage;
                info.rss = pmc.WorkingSetSize;

                FILETIME creation_time, exit_time, kernel_time, user_time;
                GetProcessTimes(GetCurrentProcess(), &creation_time, &exit_time, &kernel_time, &user_time);
                info.user_cpu_time = (user_time.dwLowDateTime / 10000000);
                info.system_cpu_time = (kernel_time.dwLowDateTime / 10000000);
#else
                std::ifstream stat_file("/proc/self/stat");
                if (stat_file) {
                    std::string tmp;
                    for (int i = 0; i < 19; ++i) {
                        stat_file >> tmp;
                    }
                stat_file >> info.vsize >> info.rss;
                }

                struct rusage usage;
                getrusage(RUSAGE_SELF, &usage);
                info.user_cpu_time = usage.ru_utime.tv_sec;
                info.system_cpu_time = usage.ru_stime.tv_sec;
                std::cout << "info.vsize = " << info.vsize << std::endl;
                std::cout << "info.rss = " << info.rss << std::endl;
                std::cout << "info.user_cpu_time = " << info.user_cpu_time << std::endl;
                std::cout << "info.system_cpu_time = " << info.system_cpu_time << std::endl;
                
#endif
                info.to_uint8_vector(buffer_out);
                data_size = info.size();
                break;
            }

        }
        return SUCCESS;
    }
    
    if (module_id_t <= modules_vtable.size() - 1) {
        if (operation_id <= modules_vtable[module_id_t].size() - 1) {
            call_signature signature = {
                .buffer_in = buffer_in.data(),
                .buf_in_size = buffer_in.size(),
                .buffer_out = buffer_out.data(),
                .buf_out_size = buffer_out.size(),
                .buffer_err = buffer_err.data(),
                .data_written = &data_size
            };
            modules_vtable[module_id_t - 1][operation_id_t](&signature);
            if (check_error(buffer_err)) {
                logger.log(Logger::LogLevel::ERROR, "Error occured in function of module "
                                                    + std::to_string(module_id_t)
                                                    + std::string(": ")
                                                    + std::string(reinterpret_cast<char*>(buffer_err.data()), buffer_err.size()));
                return ERROR_MODULE_INTERNAL;

            } else return SUCCESS;

        } else return ERROR_INVALID_OPERATION;

    } else return ERROR_INVALID_TRANSPORT;
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
    std::vector<Module> modules;
    lab_vtable modules_vtable;
    load_manifest();
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
    // Buffer for receiving data from the client
    // constexpr size_t recv_buffer_size = BUFFER_SIZE;
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
    std::vector<uint8_t> buf_in(recv_buffer.begin() + PROTOCOL_V1_HEADER_SIZE, recv_buffer.end());

    // Allocate the output buffer
    size_t buf_out_size = (static_cast<size_t>(recv_buffer[2]) + 1) * BUFFER_OUTPUT_SIZE;
    std::vector<uint8_t> buf_out(buf_out_size);

    // Allocate the error buffer
    std::vector<uint8_t> buf_err(BUFFER_ERROR_SIZE);

    // Execute the processing function
    size_t bytes_written = 0;
    LAB_STATUS exec_result = module_execute(recv_buffer[1], recv_buffer[2], buf_in, buf_out, buf_err, bytes_written);
    uint8_t response_header[4] = {recv_buffer[0], recv_buffer[1], recv_buffer[2], exec_result};
    // Prepare response data
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
    }
    CLOSE_SOCKET(client_socket);
}
