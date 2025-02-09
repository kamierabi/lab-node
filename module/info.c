#include <stdio.h>
#define u8 unsigned char
#ifdef _WIN32
#define LAB_MODULE __declspec(dllexport) void
#else
#define LAB_MODULE void
#include <sys/sysinfo.h>
#include <memory.h>
#endif


LAB_MODULE read(u8* buf_in, u8* buf_out, u8* buf_err) {
    struct sysinfo info;
    sysinfo(&info);
    memcpy(buf_out, &info, sizeof(info));
    buf_err = NULL;
}