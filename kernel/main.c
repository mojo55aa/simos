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
void k_thread_a(void *arg);
void k_thread_b(void *arg);

void (*pfunc_a)(void *) = k_thread_a;
void (*pfunc_b)(void *) = k_thread_b;

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
	// thread_start("thread A", 2, pfunc_a, "A_ ");
	// BREAK_POINT(5);
	// thread_start("thread B", 1, pfunc_b, "B_ ");
	print_bitmap();

	local_irq_enable();
	//测试系统调用
	// put_hex(getpid());
	//测试write系统调用
	// write("test systemcall write\n");
	//测试printf
	// printf(" main thread pid is 0x%x\n", getpid());

	//测试sys_malloc

	//测试跳跃分配==================
	// printf("0x%x\n", (uint32_t)sys_malloc(32));
	// printf("0x%x\n", (uint32_t)sys_malloc(64));
	// printf("0x%x\n", (uint32_t)sys_malloc(128));
	// printf("0x%x\n", (uint32_t)sys_malloc(256));

	// printf("0x%x\n", (uint32_t)sys_malloc(64));
	//=============================

	//测试sys_free==================
	void *addr1 = sys_malloc(512);
	// void* addr2 = sys_malloc(512);
	// sys_malloc(512);
	// sys_malloc(512);
	// sys_malloc(512);
	// sys_malloc(512);
	// sys_malloc(512);
	// void *addr2 = sys_malloc(512);
	print_bitmap();
	sys_free(addr1);
	// sys_free(addr2);
	// void *addr3 = sys_malloc(512);

	// printf("0x%x\n", (uint32_t)addr1);
	// printf("0x%x\n", (uint32_t)addr2);
	// printf("0x%x\n", (uint32_t)addr3);
	//=============================
	print_bitmap();

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

//测试printf
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

void k_thread_a(void* arg) {
   char* para = arg;
   void* addr1;
   void* addr2;
   void* addr3;
   void* addr4;
   void* addr5;
   void* addr6;
   void* addr7;
   console_str(" thread_a start\n");
   int max = 1000;
   while (max-- > 0) {
      int size = 128;
      addr1 = sys_malloc(size); 
      size *= 2; 
      addr2 = sys_malloc(size); 
      size *= 2; 
      addr3 = sys_malloc(size);
      sys_free(addr1);
      addr4 = sys_malloc(size);
      size *= 2; size *= 2; size *= 2; size *= 2; 
      size *= 2; size *= 2; size *= 2; 
      addr5 = sys_malloc(size);
      addr6 = sys_malloc(size);
      sys_free(addr5);
      size *= 2; 
      addr7 = sys_malloc(size);
      sys_free(addr6);
      sys_free(addr7);
      sys_free(addr2);
      sys_free(addr3);
      sys_free(addr4);
   }
   console_str(" thread_a end\n");
   while(1);
}

void k_thread_b(void* arg) {     
   char* para = arg;
   void* addr1;
   void* addr2;
   void* addr3;
   void* addr4;
   void* addr5;
   void* addr6;
   void* addr7;
   void* addr8;
   void* addr9;
   int max = 1000;
   console_str(" thread_b start\n");
   while (max-- > 0) {
      int size = 9;
      addr1 = sys_malloc(size);
      size *= 2; 
      addr2 = sys_malloc(size);
      size *= 2; 
      sys_free(addr2);
      addr3 = sys_malloc(size);
      sys_free(addr1);
      addr4 = sys_malloc(size);
      addr5 = sys_malloc(size);
      addr6 = sys_malloc(size);
      sys_free(addr5);
      size *= 2; 
      addr7 = sys_malloc(size);
      sys_free(addr6);
      sys_free(addr7);
      sys_free(addr3);
      sys_free(addr4);

      size *= 2; size *= 2; size *= 2; 
      addr1 = sys_malloc(size);
      addr2 = sys_malloc(size);
      addr3 = sys_malloc(size);
      addr4 = sys_malloc(size);
      addr5 = sys_malloc(size);
      addr6 = sys_malloc(size);
      addr7 = sys_malloc(size);
      addr8 = sys_malloc(size);
      addr9 = sys_malloc(size);
      sys_free(addr1);
      sys_free(addr2);
      sys_free(addr3);
      sys_free(addr4);
      sys_free(addr5);
      sys_free(addr6);
      sys_free(addr7);
      sys_free(addr8);
      sys_free(addr9);
   }
   console_str(" thread_b end\n");
   print_bitmap();
   while (1)
	   ;
}
