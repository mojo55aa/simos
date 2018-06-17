#ifndef __LIB_STDIO_H
#define __LIB_STDIO_H
#include "syscall.h"

#define MAX_BUFFER      1024    /*缓冲区最大长度*/
typedef char *va_list;

#define va_start(ap, v) ap = (va_list)&v /*ap指向第一个固定参数v*/
#define va_arg(ap, t) *((t *)(ap += 4))  /*ap指向下一个参数并返回值*/
#define va_end(ap) ap = NULL             /*清理ap，指向NULL*/


uint32_t vsprintf(char *res_buf, const char *format, va_list ap);
uint32_t sprintf(char *res_buf, const char *format, ...);
uint32_t printf(const char *format, ...);

#endif