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
#include "ioqueue.h"
#include "keyboard.h"
#include "syscall.h"
#include "stdio.h"

void thread_f(void *);
void thread_f_printf(void*);

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
	thread_start("thread A", 2, thread_f_printf, "A_ ");
	// BREAK_POINT(5);
	thread_start("thread B", 1, thread_f_printf, "B_ ");


	local_irq_enable();
	//测试系统调用
	// put_hex(getpid());
	//测试write系统调用
	// write("test systemcall write\n");
	//测试printf
	// printf(" main thread pid is 0x%x\n", getpid());
	while (1)
	{
		// console_str("MAIN ");
		;
	}
	return 0;
}

//测试生产者消费者
void thread_f(void *argc)
{
	char *param = argc;
	while (1)
	{
		enum intr_status old_ = local_irq_disable();
		if(!ioq_empty(&kbd_buff))
		{
			console_str(param);
			char ch = ioq_getchar(&kbd_buff);
			console_char(ch);
		}
		set_intr_status(old_);
	}
}


void thread_f_printf(void* argc)
{
	char* ch = argc;
	// printf(" thread \n");
	while(1)
	{
		// console_str(ch);
		printf("name: %s pid: %d%c",get_cur_task()->name, get_cur_task()->pid,'\n');
		;
	}
}