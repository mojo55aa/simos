#include "kstdio.h"
#include "stdio.h"
#include "global.h"
#include "console.h"

/**
 * printk --内核态格式化输出函数
*/
void printk(const char* format, ...)
{
    va_list args;
    /* format在栈中，调用va_arg()宏后就可以获取到第一个可变参数*/
    va_start(args, format);
    char res_buf[MAX_BUFFER] = {0};
    vsprintf(res_buf, format, args);
    va_end(args);
    /*将结果打印出来*/
    console_str(res_buf);
}