#include "stdio.h"
#include "global.h"
#include "string.h"
#include "debug.h"
#include "print.h"

#define va_start(ap, v) ap = (va_list)&v /*ap指向第一个固定参数v*/
#define va_arg(ap, t) *((t *)(ap += 4))  /*ap指向下一个参数并返回值*/
#define va_end(ap) ap = NULL             /*清理ap，指向NULL*/

/**
 * itoa --将整形数转换成字符串
 * @value: 待转换的整数
 * @buf: 结果缓冲区
 * @radix: 转换进制基数
*/
static void itoa(uint32_t value, char **buf, uint8_t radix)
{
    uint32_t _m = value % radix;
    uint32_t _i = value / radix;
    if (_i)
    {
        itoa(_i, buf, radix);
    }
    if (_m < 10) /*余数是0-9，转换为'0'-'9'*/
    {
        *((*buf)++) = _m + '0';
    }
    else /*余数是A-F,转换为'A'-'F'*/
    {
        *((*buf)++) = _m - 10 + 'A';
    }
}

/**
 * vsprintf --将参数ap按照format格式化输出到str缓冲区
 * 返回格式化后字符串的长度
 * @res_buf: 存储结果的缓冲区
 * @format: 格式化字符转
 * ap: 指向栈中参数的指针
*/
uint32_t vsprintf(char *res_buf, const char *format, va_list ap)
{
    char *buf_ptr = res_buf;
    const char *index_ptr = format;
    char index_char = *index_ptr;
    int32_t arg_int = 0; /*用于接收整形参数*/
    char *arg_str = "";     /*用于接收字符串类型参数*/
    while (index_char)
    {
        if (index_char != '%')
        {
            /*不是%，将当前字符写入到缓冲区*/
            *(buf_ptr++) = index_char;
            index_char = *(++index_ptr);
            continue;
        }
        /*获取%后面的字符*/
        index_char = *(++index_ptr);
        switch (index_char)
        {
            case 'x':
            {
                arg_int = va_arg(ap, int);
                itoa(arg_int, &buf_ptr, 16);
                index_char = *(++index_ptr); /*跳过格式化字符*/
                break;
            }
            case 's':
            {
                arg_str = va_arg(ap, char *);
                strcpy(buf_ptr, arg_str);
                buf_ptr += strlen(arg_str);
                index_char = *(++index_ptr);
                break;
            }
            case 'c':
            {
                *(buf_ptr++) = va_arg(ap, char);
                index_char = *(++index_ptr);
                break;
            }
            case 'd':
            {
                arg_int = va_arg(ap, int);
                if(arg_int < 0)
                {
                    arg_int = 0 - arg_int;
                    *buf_ptr++ = '-';
                }
                itoa(arg_int, &buf_ptr, 10);
                index_char = *(++index_ptr);
                break;
            }
        }
    }
    return strlen(res_buf);
}

/**
 * sprintf --格式化输出函数，输出到缓冲区
*/
uint32_t sprintf(char* res_buf, const char* format, ...)
{
    va_list args;
    uint32_t __str_len;
    va_start(args, format);
    __str_len = vsprintf(res_buf, format, args);
    va_end(args);
    return __str_len;
}

/**
 * printf --格式化输出函数,输出到终端
*/
uint32_t printf(const char *format, ...)
{
    va_list args;
    /* format在栈中，调用va_arg()宏后就可以获取到第一个可变参数*/
    va_start(args, format);
    char res_buf[MAX_BUFFER] = {0};
    vsprintf(res_buf, format, args);
    va_end(args);
    /*将结果打印出来*/
    return write(res_buf);
}