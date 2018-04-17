/**
 * issues:功能键会打印一个空格
 * solution:print.S调整put_char流程
*/
#include "keyboard.h"
#include "../include/asm/io.h"
#include "../include/asm/print.h"
#include "debug.h"
#include "interrupt.h"
#include "stdint.h"
#include "global.h"

static void key_output(uint8_t key_code, int8_t key_status);

/*屏幕输出部分*/

/*部分控制字符*/
#define esc         '\x1b'
#define enter       '\r'
#define tab         '\t'
#define backspace   '\b'
#define delete      '\x7f'

/*不可见字符*/
#define char_invisible  0
#define ctrl_r_char     char_invisible
#define ctrl_l_char     char_invisible
#define alt_r           char_invisible
#define alt_l_char      char_invisible
#define shift_r_char    char_invisible
#define shift_l_char    char_invisible
#define caps_lock_char  char_invisible

/*按键通码断码*/
#define ctrl_r_make     0xe01d
#define ctrl_r_break    0xe09d
#define ctrl_l_make     0x1d
#define ctrl_l_break    0x9d
#define alt_r_make      0xe038
#define alt_r_break     0xe0b8
#define alt_l_make      0x38
#define alt_l_break     0xb8
#define shift_r_make    0x36
#define shift_r_break   0xb6
#define shift_l_make    0x2a
#define shift_l_break   0xaa
#define caps_lock_make  0x3a
#define caps_lock_break 0xba

/*可打印字符与通码映射表*/
static char keymap[][2] = {
    /*未与shift组合*/   /*与shift组合*/
    /* 0x00 */      {0, 0},
    /* 0x01 */      {esc, esc},
    /* 0x02 */      {'1', '!'},
    /* 0x03 */      {'2', '@'},
    /* 0x04 */      {'3', '#'},
    /* 0x05 */      {'4', '$'},
    /* 0x06 */      {'5', '%'},
    /* 0x07 */      {'6', '^'},
    /* 0x08 */      {'7', '&'},
    /* 0x09 */      {'8', '*'},
    /* 0x0A */      {'9', '('},
    /* 0x0B */      {'0', ')'},
    /* 0x0C */      {'-', '_'},
    /* 0x0D */      {'=', '+'},
    /* 0x0E */      {backspace, backspace},
    /* 0x0F */      {tab, tab},
    /* 0x10 */      {'q', 'Q'},
    /* 0x11 */      {'w', 'W'},
    /* 0x12 */      {'e', 'E'},
    /* 0x13 */      {'r', 'R'},
    /* 0x14 */      {'t', 'T'},
    /* 0x15 */      {'y', 'Y'},
    /* 0x16 */      {'u', 'U'},
    /* 0x17 */      {'i', 'I'},
    /* 0x18 */      {'o', 'O'},
    /* 0x19 */      {'p', 'P'},
    /* 0x1A */      {'[', '{'},
    /* 0x1B */      {']', '}'},
    /* 0x1C */      {enter, enter},
    /* 0x1D */      {ctrl_l_char, ctrl_l_char},
    /* 0x1E */      {'a', 'A'},
    /* 0x1F */      {'s', 'S'},
    /* 0x20 */      {'d', 'D'},
    /* 0x21 */      {'f', 'F'},
    /* 0x22 */      {'g', 'G'},
    /* 0x23 */      {'h', 'H'},
    /* 0x24 */      {'j', 'J'},
    /* 0x25 */      {'k', 'K'},
    /* 0x26 */      {'l', 'L'},
    /* 0x27 */      {';', ':'},
    /* 0x28 */      {'\'', '"'},
    /* 0x29 */      {'`', '~'},
    /* 0x2A */      {shift_l_char, shift_l_char},
    /* 0x2B */      {'\\', '|'},
    /* 0x2C */      {'z', 'Z'},
    /* 0x2D */      {'x', 'X'},
    /* 0x2E */      {'c', 'C'},
    /* 0x2F */      {'v', 'V'},
    /* 0x30 */      {'b', 'B'},
    /* 0x31 */      {'n', 'N'},
    /* 0x32 */      {'m', 'M'},
    /* 0x33 */      {',', '<'},
    /* 0x34 */      {'.', '>'},
    /* 0x35 */      {'/', '?'},
    /* 0x36	*/      {shift_r_char, shift_r_char},
    /* 0x37 */      {'*', '*'},
    /* 0x38 */      {alt_l_char, alt_l_char},
    /* 0x39 */      {' ', ' '},
    /* 0x3A */      {caps_lock_char, caps_lock_char},
    /* 0x3B */      {0, 0},
    /* 0x3C */      {0, 0},
    /* 0x3D */      {0, 0},
    /* 0x3E */      {0, 0},
    /* 0x3F */      {0, 0},
    /* 0x40 */      {0, 0},
    /* 0x41 */      {0, 0},
    /* 0x42 */      {0, 0},
    /* 0x43 */      {0, 0},
    /* 0x44 */      {0, 0},
    /* 0x45 */      {0, 0},
    /* 0x46 */      {0, 0},
    /* 小键盘部分 */
    /* 0x47 */      {'7', '7'},
    /* 0x48 */      {'8', '8'},
    /* 0x49 */      {'9', '9'},
    /* 0x4A */      {'-', '-'},
    /* 0x4B */      {'4', '4'},
    /* 0x4C */      {'5', '5'},
    /* 0x4D */      {'6', '6'},
    /* 0x4E */      {'+', '+'},
    /* 0x4F */      {'1', '1'},
    /* 0x50 */      {'2', '2'},
    /* 0x51 */      {'3', '3'},
    /* 0x52 */      {'0', '0'},
    /* 0X53 */      {'.', '.'}
};


int8_t g_key_status = 0;  /*键盘按键状态，低位到高位一次表示shift_l,ctrl_l,alt_l,
                                                     shift_r,ctrl_r,alt_r,
                                                     caps_lock,ext_code*/

#define SET_BIT(operand,nbit)       (operand | (1 << nbit))
#define CLEAR_BIT(operand,nbit)     (operand & (~(1 << nbit)))
#define GET_BIT(operand,nbit)       ((operand & (1 << nbit)) >> nbit)
/**
 * keyboard_service --键盘中断服务程序
 */
static void keyboard_service(void) {
    uint16_t __scancode = inb(KBD_BUF_PORT);
    /*如果扫描码是0xe0开头，退出，接收下一个扫描码*/
    if(__scancode == 0xe0)
    {
        g_key_status = SET_BIT(g_key_status, 7); /*ext_code位置1*/
        return;
    }
    /*0xe0标记清除,扫描码与0xe0合并*/
    if (g_key_status & 0x80)
    {
        __scancode += 0xe000;
        g_key_status = CLEAR_BIT(g_key_status, 7);
    }

    /*判断是否是功能键*/
    switch(__scancode & 0x00ff) /*取低字节*/
    {
        /*通码，g_key_status置位*/
        case ctrl_l_make:{
            if(__scancode & 0xff00)
            {
                g_key_status = SET_BIT(g_key_status, 4);
            }
            else
                g_key_status = SET_BIT(g_key_status, 1);
            break;
        }
        case alt_l_make:{
            if(__scancode & 0xff00)
                g_key_status = SET_BIT(g_key_status, 5);
            else
                g_key_status = SET_BIT(g_key_status, 2);
            break;
        }
        case shift_l_make:{
            g_key_status = SET_BIT(g_key_status, 0);
            break;
        }
        case shift_r_make:{
            g_key_status = SET_BIT(g_key_status, 3);
            break;
        }
        case caps_lock_make:{
            if(g_key_status & 0x40)
                g_key_status = CLEAR_BIT(g_key_status, 6);
            else
                g_key_status = SET_BIT(g_key_status, 6);
            break;
        }
        /*断码，清除标志位*/
        case ctrl_l_break:{
            if(__scancode & 0xff00)
                g_key_status = CLEAR_BIT(g_key_status, 4);
            else
                g_key_status = CLEAR_BIT(g_key_status, 1);
        }
        case alt_l_break:{
            if(__scancode & 0xff00)
                g_key_status = CLEAR_BIT(g_key_status, 5);
            else
                g_key_status = CLEAR_BIT(g_key_status, 2);
            break;
        }
        case shift_l_break:{
            g_key_status = CLEAR_BIT(g_key_status, 0);
            break;
        }
        case shift_r_break:{
            g_key_status = CLEAR_BIT(g_key_status, 3);
        }
    }
    /*如果是通码，输出*/
    if (!((__scancode & 0x00ff) >> 7))
    {
        key_output(__scancode & 0x00ff, g_key_status);
    }
}

/**
 * key_output --向屏幕输出按键信息
*/
static void key_output(uint8_t key_code, int8_t key_status)
{
    /*获取shift与caps_lock按键状态*/
    uint8_t __shift = GET_BIT(key_status, 0) | GET_BIT(key_status, 3);
    uint8_t __caps_lock = GET_BIT(key_status, 6);
    
    if(__caps_lock)     /*caps_lock处于激活状态*/
    {
        __shift ? put_char(keymap[key_code][0]) : put_char(keymap[key_code][1]);
    }
    else
    {
        __shift ? put_char(keymap[key_code][1]) : put_char(keymap[key_code][0]);
    }
}

/*初始化键盘*/
void keyboard_init(void) {
    //注册中断服务程序
    put_str("Keyboard initialization start\n");
    register_irq_handler(KEYBOARD_VECTOR, keyboard_service);
    put_str("Keyboard initialization completion\n");
}