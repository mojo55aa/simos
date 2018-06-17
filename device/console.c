#include "../include/os/console.h"
#include "../include/os/sync.h"
#include "../include/asm/print.h"
#include "../include/os/stdint.h"

static struct lock console_lock;

void console_init()
{
    lock_init(&console_lock);
}

static void console_acquire()
{
    lock_acquire(&console_lock);
}

static void console_release()
{
    lock_release(&console_lock);
}

void console_str(char* str)
{
    console_acquire();
    put_str(str);
    console_release();
}

void console_char(char ch)
{
    console_acquire();
    put_char(ch);
    console_release();
}

void console_hex(uint32_t hex)
{
    console_acquire();
    put_hex(hex);
    console_release();
}