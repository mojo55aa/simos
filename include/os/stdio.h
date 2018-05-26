#ifndef __LIB_STDIO_H
#define __LIB_STDIO_H
#include "syscall.h"

#define MAX_BUFFER      1024    /*缓冲区最大长度*/
typedef char *va_list;

uint32_t sprintf(char *res_buf, const char *format, ...);
uint32_t printf(const char *format, ...);

#endif