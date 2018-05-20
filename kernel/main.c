/*ISSUES
*(1) 打印整数函数put_int	--print.S
*(2) local_irq_save(),local_irq_restore()待重写		--interrupt.h
*/

#include "print.h"
#include "init.h"
#include "interrupt.h"
#include "debug.h"
#include "thread.h"
#include "console.h"

void thread_f(void *);

int main(void)
{
	put_str("total memory: 0x");
	uint32_t total_mem = (*(uint32_t *)(0xb00));
	put_hex(total_mem);
	put_char('\n');
	init_os();
	//测试缺页异常
	// uint32_t x = *(uint32_t *)(0x15000);
	// BREAK_POINT(6);
	thread_start("thread A", 2, thread_f, "threadA ");
	// BREAK_POINT(5);
	thread_start("thread B", 1, thread_f, "threadB ");
	local_irq_enable();
	while (1)
	{
		console_str("MAIN ");
	}
	return 0;
}

void thread_f(void *argc)
{
	char *param = argc;
	while (1)
	{
		console_str(param);
	}
}
