#ifndef __DEVICE_CONSOLE_H
#define __DEVICE_CONSOLE_H
#include "stdint.h"

void console_init();
void console_str(char *str);
void console_char(char ch);
void console_hex(uint32_t hex);


#endif