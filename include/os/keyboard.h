#ifndef __DEVICE_KEYBOARD_H
#define __DEVICE_KEYBOARD_H
#include "ioqueue.h"

#define KBD_BUF_PORT 0x60   /*键盘端口*/
#define KEYBOARD_VECTOR 0x21    /*键盘中断*/

extern struct ioqueue kbd_buff;
void keyboard_init(void);

#endif