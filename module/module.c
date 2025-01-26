#include <stdio.h>
#define u8 unsigned char

void read(u8* buf_in, u8* buf_out, u8* buf_err) {
    buf_out[0] = 0x48;
    buf_out[1] = 0x69;
    buf_out[2] = 0x21;
    buf_err = NULL;
}