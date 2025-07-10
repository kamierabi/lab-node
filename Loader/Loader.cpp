#include "Loader.hpp"
#include <sstream>
#include <cstring>

Loader::Loader(const std::string& dir_path)
{
    for (const auto& entry : fs::directory_iterator(dir_path)) {
        if (entry.is_regular_file()) {
            auto ext = entry.path().extension().string();
            #ifdef _WIN32
                if (ext == ".dll")
            #else
                if (ext == ".so")
            #endif
            {
                std::string module_path = entry.path().string();
                modules.emplace_back(Module(module_path));
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

int Loader::system_loader_info(call_signature* args)
{
    std::ostringstream oss;
    for (const auto& mod : modules) {
        oss << mod.get_module_name();
        for (size_t i = 0; i < mod.metadata->func_count; ++i) {
            oss << '\t' << mod.metadata->functions_sym[i];
        }
        oss << '\n';
    }
    std::string tsv = oss.str();
    if (tsv.size() <= args->buf_out_size) {
        std::memcpy(args->buffer_out, tsv.c_str(), tsv.size());
        *(args->data_written) += tsv.size();
        return 0;
    } else {
        return 1;
    }
}

int Loader::execute(
    size_t module_id,
    size_t function_id,
    std::vector<uint8_t>& buffer_in,
    std::vector<uint8_t>& buffer_out,
    std::vector<uint8_t>& buffer_err,
    size_t& data_size
) {
    call_signature signature = {
        buffer_in.data(),
        buffer_in.size(),
        buffer_out.data(),
        buffer_out.size(),
        buffer_err.data(),
        &data_size
    };

    if (module_id == 255) {
        switch (function_id) {
            case 1:
                return system_loader_info(&signature);
            default:
                return system_echo(&signature);
        }
    } else {
        if (module_id >= modules.size()) return 3;
        if (function_id >= modules[module_id].functions.size()) return 4;
        return modules[module_id].functions[function_id](&signature);
    }
}
