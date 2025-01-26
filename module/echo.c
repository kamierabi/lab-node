#include <stdio.h>
#include <memory.h>
#define u8 unsigned char
#ifdef _WIN32
#define LAB_MODULE __declspec(dllexport)
#else
#define LAB_MODULE
#endif

void LAB_MODULE read(u8* buf_in, u8* buf_out, u8* buf_err) {
    memcpy(buf_out, buf_in, sizeof(buf_in));
    buf_err = NULL;
}

