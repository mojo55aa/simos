#include "string.h"
#include "debug.h"
#include "global.h"

/**
 * WARNING: 所有字符串操作函数都没有进行缓冲区大小检查，
 * 意味着可能发生缓冲区溢出，使用时需要保证内存块不重叠以及大小正确
*/

/** 
 * memset --把一段内存初始化成给定的值.
 * @_dst: 内存起始地址.
 * @ch: the byte to fill the area with.
 * @count: the size of the area.
 * return: pointer to the start of the area.
 * */
void* memset(void* _dst, uint8_t ch, uint32_t count)
{
    ASSERT(_dst != NULL);
    char* dst = (char*)_dst;

    while(count--)
        *dst++ = ch;

    return _dst;
}

/**
 * memcpy --copy one area of memory to another.
 * @_dst: the pointer of where to copy to.
 * @_src: the pointer of where to copy from.
 * @count: the size of the area.
 * return: pointer to the start of the destination. 
 * */
void* memcpy(void* _dst, const void* _src, uint32_t count)
{
    ASSERT(_dst != NULL && _src != NULL);
    char* dst = (char*)_dst;
    char* src = (char*)_src;

    while(count--)
        *dst++ = *src++;

    return _dst;
}

/**
 * memcmp --比较两个buffer是否相等
 * @_buf1: buffer1.
 * @_buf2: buffer2.
 * @count: the size of the buffer.
 * return: buf1==buf2 返回0，buf1<buf2 返回<0,buf1>buf2 返回>0
*/
int memcmp(const void* _buf1, const void* _buf2, uint32_t count)
{
    ASSERT(_buf1 != NULL && _buf2 != NULL);
    const char *buf1,*buf2;
    int res = 0;

    for(buf1 = _buf1, buf2 = _buf2; count > 0; ++buf1, ++buf2,count--)
    {
        if((res = *buf1 - *buf2) != 0)
            break;
    }

    return res;
}

/**
 * strcpy --将原字符串拷贝到目的地址
 * @_dst: where to copy the string to
 * @_drc: where to copy the string from
 * return: 拷贝后字符串的地址
 */
char* strcpy(char* _dst, const char* _src)
{
    ASSERT(_dst != NULL && _src != NULL);
    char* tmp = _dst;

    while((*tmp++ = *_src++) != '\0')
        ;
    /*nothing to do*/
    return _dst;
}

/**
 * strcmp --比较两个字符串是否相等
 * @_str1: one string
 * @_str2: another string
 * return: 相等返回0，str1<str2,返回值<0;str1>str2,返回值大于0
*/
int strcmp(const char* _str1, const char* _str2)
{
    ASSERT(_str1 != NULL && _str2 != NULL);

    while(*_str1 == *_str2)
    {
        if(*_str1 == '\0')
            break;
        _str1++;
        _str2++;
    }

    return (*_str1 - *_str2);
}

/**
 * strlen --返回字符串长度
 * @_str: the string to be sized
 * return: length of the string
 */
uint32_t strlen(const char* _str)
{
    ASSERT(_str != NULL);
    uint32_t count = 0;

    while(*_str++ != '\0')
        count++;
    
    return count;
}

/**
 * strchr --查找字符第一次在字符串中从左面开始出现的位置
 * @_str: the string to be searched.
 * @ch: the character to search for.
 * return: the pointer point to where the character is.
*/
char* strchr(const char* _str, const char ch)
{
    ASSERT(_str != NULL);

    for(; *_str != ch; ++_str)
    {
        if(*_str == '\0')
            return NULL;
    }
    return (char*)_str;
}

/**
 * strrchr --查找字符在字符串中从右面开始第一次出现的位置
 * @_str: the string to be searched
 * @ch: the character to search for
 * return: the pointer point to where the character is
*/
char* strrchr(const char* _str, const char ch)
{
    ASSERT(_str != NULL);
    const char* p = _str + strlen(_str);

    do{
        if(*p == ch)
            return (char*)p;
    }while(--p >= _str);

    return NULL;
}

/**
 * strcat --拼接字符串
 * @_dst: the string to be appended to
 * @_src： the string to appended to it
 * return: pointer to the destination string 
*/
char* strcat(char* _dst, const char* _src)
{
    ASSERT(_dst != NULL && _src != NULL);
    char* __p = _dst;

    while(*__p)
        __p++;
    while((*__p++ = *_src++) != '\0')
        ;
    /*do nothing*/

    return _dst;
}

/**
 * strchrs --返回字符ch在字符串中出现的次数
 * @_str: the string to be searched
 * @ch: the character to search for
 * return: the number of times the character appears
*/
uint32_t strchrs(const char* _str, const char ch)
{
    ASSERT(_str != NULL);
    uint32_t __res = 0;

    for (; *_str != '\0'; ++_str)
    {
        if(*_str == ch)
            __res++;
    }

    return __res;
}