#include "timer.h"
#include "interrupt.h"
#include "print.h"
#include "thread.h"
#include "debug.h"

uint32_t ticks = 0;
/**
 * clock_service --时钟中断服务程序
*/
void clock_service()
{
    struct task_struct *cur_thread = current_task;

    put_hex((uint32_t)current_task);
    put_str("function:clock_service\n");
    put_str(cur_thread->name);
    PRINT_ADDR(" ", (uint32_t)cur_thread);
    put_hex(cur_thread->stack_boundary);
    put_char('\n');

    // ASSERT(cur_thread->stack_boundary == 0x20130901);
    ASSERT(6 == 6);
    cur_thread->elapsed_ticks++;
    ticks++;
    if(cur_thread->ticks == 0)
    {
        put_str(" schedule ");
        schedule(); /*时间片用完，执行调度函数*/
    }
    else
    {
        cur_thread->ticks--;
    }
}

void timer_init()
{
    register_irq_handler(CLOCK_VECTOR, clock_service);
    put_str("register clock interruption service done\n");
}