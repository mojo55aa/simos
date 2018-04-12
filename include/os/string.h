#ifndef __LIB_STRING_H
#define __LIB_STRING_H
#include "stdint.h"

/*内存操作*/
void* memset(void* _dst, uint8_t ch, uint32_t count);
int memcmp(const void* _buf1, const void* _buf2, uint32_t count);
void* memcpy(void* _dst, const void* _src, uint32_t count);

/*字符串操作*/
char* strcpy(char* _dst, const char* _src);
uint32_t strlen(const char* _str);
int strcmp(const char* _str1, const char* _str2);
char* strchr(const char* _strt, const char ch);

char* strrchr(const char* _str, const char ch);
char* strcat(char* _dst, const char* _src);
uint32_t strchrs(const char* _str, const char ch);

#endif