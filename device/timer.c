/**
 * ISSUSE
 * 1s发生了10000次时钟中断?
*/
#include "timer.h"
#include "interrupt.h"
#include "print.h"
#include "thread.h"
#include "debug.h"
#include "io.h"
#include "kstdio.h"


#define IRQ0_FREQUENCY	   100      /* 1s发生100次时钟中断 */
#define INPUT_FREQUENCY	   1193180
#define COUNTER0_VALUE	   INPUT_FREQUENCY / IRQ0_FREQUENCY
#define CONTRER0_PORT	   0x40
#define COUNTER0_NO	   0
#define COUNTER_MODE	   2
#define READ_WRITE_LATCH   3
#define PIT_CONTROL_PORT   0x43

/* 发生时钟中断的毫秒间隔 */
#define millisecond_per_intr    (1000 / IRQ0_FREQUENCY)



/* 内核从开中断开始一共的滴答数 */
uint32_t ticks = 0;
/**
 * clock_service --时钟中断服务程序
 * 任务调度
*/
void clock_service()
{
    struct task_struct *cur_thread = get_cur_task();

    ASSERT(cur_thread->stack_boundary == 0x20130901);

    cur_thread->elapsed_ticks++;
    ticks++;
    if(cur_thread->ticks == 0)
    {
        schedule(); /*时间片用完，执行调度函数*/
    }
    else
    {
        cur_thread->ticks--;
    }
}

static void frequency_set(uint8_t counter_port, \
			  uint8_t counter_no, \
			  uint8_t rwl, \
			  uint8_t counter_mode, \
			  uint16_t counter_value) {
/* 往控制字寄存器端口0x43中写入控制字 */
   outb(PIT_CONTROL_PORT, (uint8_t)(counter_no << 6 | rwl << 4 | counter_mode << 1));
/* 先写入counter_value的低8位 */
   outb(counter_port, (uint8_t)counter_value);
/* 再写入counter_value的高8位 */
   outb(counter_port, (uint8_t)counter_value >> 8);
}

static void ticks_to_sleep(uint32_t sleep_ticks)
{
    uint32_t start_ticks = ticks;
    /* 记录睡眠时刻开始的系统ticks,如果ticks不够，则进程被
    再次调度上CPU后会继续让出CPU，直到时间片到 */
    // printk("start_ticks %d\n", ticks);
    while (ticks - start_ticks < sleep_ticks)
    {
        thread_yield();
    }
    // printk("end_ticks %d\n", ticks);
}

/**
 * mil_sleep --以毫秒为单位的睡眠
 * @m_second: 睡眠时间，单位毫秒
*/
void mil_sleep(uint32_t m_second)
{
    uint32_t sleep_ticks = DIV_ROUND_UP(m_second * 100, millisecond_per_intr);
    ASSERT(sleep_ticks > 0);
    ticks_to_sleep(sleep_ticks);
}

void timer_init()
{
    /* 设置8253的定时周期 */
    frequency_set(CONTRER0_PORT, COUNTER0_NO, READ_WRITE_LATCH, COUNTER_MODE, COUNTER0_VALUE);
    register_irq_handler(CLOCK_VECTOR, clock_service);
    put_str("register clock interruption service done\n");
}