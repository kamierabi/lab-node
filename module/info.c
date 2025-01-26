#include <stdio.h>
#include <sys/sysinfo.h>
#define u8 unsigned char
#ifdef _WIN32
#define LAB_MODULE __declspec(dllexport)
#else
#define LAB_MODULE
#endif

