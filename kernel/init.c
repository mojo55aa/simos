#include "init.h"
#include "print.h"
#include "interrupt.h"
#include "keyboard.h"
#include "memory.h"

void init_os(void)
{
    put_str("Start Initialize System\n");
    interrupt_init();
    keyboard_init();
    mem_init();
}