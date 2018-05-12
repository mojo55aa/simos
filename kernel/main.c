/*ISSUES
*(1) 打印整数函数put_int	--print.S
*(2) local_irq_save(),local_irq_restore()待重写		--interrupt.h
*/

#include "print.h"
#include "init.h"
#include "interrupt.h"
#include "debug.h"
#include "thread.h"

void thread_a(void *);
void thread_b(void *);

int main(void) {
	put_str("total memory: 0x");
	uint32_t total_mem = (*(uint32_t *)(0xb00));
	put_hex(total_mem);
	put_char('\n');
	init_os();
	//测试缺页异常
	// uint32_t x = *(uint32_t *)(0x15000);
	// BREAK_POINT(6);
	thread_start("thread A", 31, thread_a, "argA ");
	// BREAK_POINT(5);
	thread_start("thread B", 8, thread_b, "argB ");
	local_irq_enable();
	while (1)
		put_str("MAIN ");
	return 0;
}

void thread_a(void* argc)
{
	char* param = argc;
	put_str(param);
	while(1)
		;
}

void thread_b(void* argc)
{
	char* param = argc;
	put_str(param);
	while(1)
		;
}