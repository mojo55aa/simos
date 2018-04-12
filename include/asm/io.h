/*
*封装端口读写操作
*outb   向端口写入一字节数据
*outsw  将n个字的数据写入端口
*inb    从端口读一字节数据
*insw   将端口返回的n个字的数据写入add起始地址处
*/
#ifndef __ASM_IO_H
#define __ASM_IO_H
#include "stdint.h"

/*向端口port写入一字节数据*/
static inline void outb(uint16_t port, uint16_t data)
{
    asm volatile ("outb %b0, %w1" : : "a" (data), "Nd" (port));
}

/*将addr处起始的word_cnt个字写入port*/
static inline void outsw(uint16_t port, const void* addr, uint32_t word_cnt)
{
    asm volatile ("cld; rep outsw" : "+S" (addr),"+c" (word_cnt) : "d" (port));
}

/*从端口port读出一字节数据返回*/
static inline uint8_t inb(uint16_t port)
{
    uint8_t data;
    asm volatile ("inb %w1, %b0" : "=a" (data) : "Nd" (port));
    return data;
}

/*将从端口port读出的word_cnt个字写入addr地址处*/
static inline void insw(uint16_t port, void* addr, uint32_t word_cnt)
{
    asm volatile ("cld; rep insw" : "+D" (addr), "+c" (word_cnt) : "d" (port) : "memory");
}

#endif