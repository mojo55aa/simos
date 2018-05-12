#ifndef __KERNEL_PRINT_H
#define __KERNEL_PRINT_H
#include "../os/stdint.h"
void put_char(uint8_t char_asci);
void put_str(char* str);
void put_hex(uint32_t hex);
#endif