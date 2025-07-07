#pragma once
#include <iostream>
#include <unordered_map>
#include <dlfcn.h>
#include <functional>
#include <filesystem>
#include <cstring>

#ifdef _WIN32
    #define MODULE_HANDLE HMODULE
    #define LOAD_MODULE(path) LoadLibrary(path)
    #define FREE_MODULE FreeLibrary(handle)
    #define GET_SYMBOL(handle, symbol) GetProcAddress(handle, symbol)
    #define MODULE_ERROR_t DWORD
    #define GET_ERROR() GetLastError()
#else
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


// Metadata for loading arguments 
// struct {
//     uint8_t type_id;
//     uint8_t size;
//     uint32_t offset;
//     uint32_t capacity;
// } offset_entry_t;

enum TypeId 
{
    TYPE_I8 = 0x01,
    TYPE_U8,
    TYPE_I16,
    TYPE_U16,
    TYPE_I32,
    TYPE_U32,
    TYPE_I64,
    TYPE_U64,
    TYPE_FLOAT,
    TYPE_DOUBLE,
    TYPE_BOOL,
    TYPE_STRING
};

// Metadata for loading modules

struct module_metadata
{
    const char* module_id;
    size_t func_count;
    const char** functions_sym;
};

namespace fs = std::filesystem;
typedef int (*func_type)(call_signature* args);


struct Module
{
    void* handle;
    module_metadata* metadata;
    std::vector<std::function<int(call_signature*)>> functions;

    // Конструктор
    Module(const std::string& module_path) {
        dlerror();
        handle = dlopen(module_path.c_str(), RTLD_NOW);
        if (!handle) {
            throw std::runtime_error("Could not open library: " + std::string(dlerror()));
        }

        metadata = reinterpret_cast<module_metadata*>(dlsym(handle, "__metadata__"));
        const char* err = dlerror();
        if (err) {
            throw std::runtime_error("Failed to load metadata: " + std::string(err));
        }

        for (size_t i = 0; i <= metadata->func_count-1; ++i) {
            func_type func_ptr = reinterpret_cast<func_type>(dlsym(handle, metadata->functions_sym[i]));
            const char* func_err = dlerror();
            if (func_err) {
                throw std::runtime_error("Failed to load function: " + std::string(func_err));
            }
            functions.emplace_back(func_ptr); // std::function из указателя
        }
    }

    // Перемещающий конструктор
    Module(Module&& other) noexcept
        : handle(other.handle), metadata(other.metadata), functions(std::move(other.functions)) {
        other.handle = nullptr;
        other.metadata = nullptr;
    }

    // Запрещаем копирование
    Module(const Module&) = delete;
    Module& operator=(const Module&) = delete;
    
    // Перемещающее присваивание
    Module& operator=(Module&& other) noexcept {
        if (this != &other) {
            // Закрыть старый handle
            if (handle) dlclose(handle);
            handle = other.handle;
            metadata = other.metadata;
            functions = std::move(other.functions);

            other.handle = nullptr;
            other.metadata = nullptr;
        }
        return *this;
    }

    ~Module() {
        if (handle) {
            dlclose(handle);
        }
    }

    std::string get_module_name() const {
        if (!metadata) {
            throw std::runtime_error("Invalid metadata access");
        }
        return std::string(metadata->module_id);
    }

    std::string get_function_name(size_t n_func) const {
        if (!metadata) {
            throw std::runtime_error("Invalid metadata access");
        }
        return std::string(metadata->functions_sym[n_func]);
    }

};

struct Loader
{
    std::vector<Module> modules;
    std::string modules_repr;
    size_t module_counter = 0;
    Loader(const std::string& dir_path);

    int system_echo(call_signature* args);
    // int system_proc_info(call_signature* args);
    int system_loader_info(call_signature* args);
    int execute(
        size_t module_id,
        size_t function_id,
        std::vector<uint8_t>& buffer_in,
        std::vector<uint8_t>& buffer_out,
        std::vector<uint8_t>& buffer_err,
        size_t& data_size
    );
};