#include <iostream>
#include <unordered_map>
#include <dlfcn.h>
#include <functional>
#include <filesystem>
#include <cstring>
#include "Loader.hpp"


Loader::Loader(const std::string& dir_path)
{
    for (const auto& entry : fs::directory_iterator(dir_path)) {
        if (entry.is_regular_file()) {
            auto ext = entry.path().extension().string();
            if (ext == ".so") {
                std::string module_path = entry.path();
                modules.emplace_back(Module(module_path)); // перемещение!
                ++module_counter;
            }
        }
    }
}

int Loader::system_echo(call_signature* args)
{
    std::memcpy(args->buffer_out, args->buffer_in, args->buf_in_size);
    *(args->data_written) = args->buf_in_size;
    return 0;
}

int Loader::system_proc_info(call_signature* args)
{
    return 1;
}
int Loader::system_loader_info(call_signature* args)
{
    std::ostringstream oss;
    for (const auto& mod : modules) {
        oss << (mod.get_module_name());
        for (size_t i = 0; i <= mod.metadata->func_count; ++i) {
            oss << '\t' << mod.metadata->functions_sym[i];
        }
    }
    oss << '\n';
    auto tsv = oss.str();
    if (tsv.size() <= args->buf_out_size) {
        std::memcpy(args->buffer_out, tsv.c_str(), tsv.size());
        *(args->data_written) += tsv.size();
        return 0;
    }
    else {
        return 1;
    }
}


int Loader::execute
    (
        size_t module_id,
        size_t function_id,
        std::vector<uint8_t>& buffer_in,
        std::vector<uint8_t>& buffer_out,
        std::vector<uint8_t>& buffer_err,
        size_t& data_size
    ) 
{
    call_signature signature = {
                .buffer_in = buffer_in.data(),
                .buf_in_size = buffer_in.size(),
                .buffer_out = buffer_out.data(),
                .buf_out_size = buffer_out.size(),
                .buffer_err = buffer_err.data(),
                .data_written = &data_size
    };
    if (module_id == 0) {
        int res;
        switch (function_id)
        {
        default:
            res = system_echo(&signature);
            break;
        }
        return res;
    }
    else {
        if (module_id > module_counter) return 3;
        if (function_id > modules[module_id].functions.size() - 1) return 4; 
        int result = modules[module_id].functions[function_id](&signature);
        return result;
    }
}
