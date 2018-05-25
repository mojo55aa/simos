#include "init.h"
#include "print.h"
#include "interrupt.h"
#include "keyboard.h"
#include "memory.h"
#include "timer.h"
#include "thread.h"
#include "console.h"
#include "tss.h"

void init_os(void)
{
    put_str("Start Initialize System\n");
    interrupt_init();
    keyboard_init();
    mem_init();
    timer_init();   /*为进程注册时钟中断处理程序*/
    thread_init();
    console_init();
    tss_init();
}