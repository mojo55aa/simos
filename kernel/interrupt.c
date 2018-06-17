/*
this file responsible for initialization interrupt
step 1.	初始化中断描述符表
step 2.	初始化8259A
step 3. 加载idt
*/
#include "interrupt.h"
#include "stdint.h"
#include "global.h"
#include "io.h"
#include "print.h"
#include "debug.h"

#define PIC_M_CTRL	0x20		//主片控制端口
#define PIC_M_DATA	0x21		//主片数据端口
#define PIC_S_CTRL	0xa0		//从片控制端口
#define PIC_S_DATA	0xa1		//从片数据端口

#define IDT_DESC_CNT 0x81	//中断数量

#define SYSCALL_NUMBER	0x80	//系统调用中断号

//中断门描述符结构体
struct gate_descr{
	uint16_t	func_offest_low_word;	//中断处理程序offset 15-0位
	uint16_t	selector;				//目标代码段描述符选择子
	uint8_t		dcount;					//固定未使用
	uint8_t		attribute;				/*属性位 |P|DPL|S|TYPE|
											TYPE:1110--32位中断门描述符
											S:0 (0:系统段，1：数据段)*/

	uint16_t 	func_offest_high_word;	//中断处理程序offset 31-16位
};

static struct gate_descr idt[IDT_DESC_CNT];		//中断描述符表
intr_handler irqaction_t[IDT_DESC_CNT];	//中断服务程序数组
extern intr_handler intr_entry_table[IDT_DESC_CNT];	//中断处理函数入口数组
char* trap_name[IDT_DESC_CNT];		//保存中断名字
extern uint32_t syscall_handler(void);	//系统调用入口函数，在interrupt_s.S中定义

//通用中断处理程序
static void ignore_intr_handler(uint8_t ver_n)
{
	if(ver_n == 0x27 || ver_n == 0x2f)
	{
		return;
	}
	if(ver_n == 14)
	{
		put_str("\nPage Fault CR2: 0x");
		uint32_t address = 0;
		asm volatile("movl %%cr2,%0"
					 : "=r"(address));
		put_hex(address);
	}
	put_char('\n');
	put_hex(ver_n);
	put_str(trap_name[ver_n]);

	while(1)
		;
}

//注册通用中断服务程序
static void trap_init()
{
	int i;
	for(i = 0;i < IDT_DESC_CNT;i++)
	{
		irqaction_t[i] = ignore_intr_handler;
		trap_name[i] = "Unknow interrupt";
	}
															//描述					  //类型	  //错误码

	trap_name[0] = "#DE Divide Error";						//除法错					故障		N
   	trap_name[1] = "#DB Debug Exception";					//调试异常					
   	trap_name[2] = "NMI Interrupt";							//NMI中断					中断		N
   	trap_name[3] = "#BP Breakpoint Exception";				//断点						陷阱		N
   	trap_name[4] = "#OF Overflow Exception";				//溢出						陷阱		N
   	trap_name[5] = "#BR BOUND Range Exceeded Exception";	//数组引用超出边界			  故障		  N
   	trap_name[6] = "#UD Invalid Opcode Exception";			//无效或未定义操作码		  故障		  N
   	trap_name[7] = "#NM Device Not Available Exception";	//设备不可用				 故障		 N
   	trap_name[8] = "#DF Double Fault Exception";			//双重故障					终止		Y
   	trap_name[9] = "Coprocessor Segment Overrun";			//协处理器段超越			  故障		  N
   	trap_name[10] = "#TS Invalid TSS Exception";			//无效TSS					故障		Y
   	trap_name[11] = "#NP Segment Not Present";				//段不存在					故障		 Y
   	trap_name[12] = "#SS Stack Fault Exception";			//堆栈异常					故障		 Y
   	trap_name[13] = "#GP General Protection Exception";		//一般保护异常				 故障		  Y
   	trap_name[14] = "#PF Page-Fault Exception";				//缺页异常					故障		 Y
   	//第15项是intel保留项，未使用																		N
   	trap_name[16] = "#MF x87 FPU Floating-Point Error";		//浮点处理器错误			  故障		   N
   	trap_name[17] = "#AC Alignment Check Exception";		//对齐检查					故障		  Y
   	trap_name[18] = "#MC Machine-Check Exception";			//机器检查					终止		  N
   	trap_name[19] = "#XF SIMD Floating-Point Exception";	//SIMD浮点异常				故障			N
	trap_name[0x20] = "clock Interrupt";
	trap_name[0x21] = "keyboard Interrupt";
}

//注册中断服务程序
void register_irq_handler(uint8_t vector_no, IRQ_HANDLER handler)
{
	ASSERT(handler != NULL);
	//中断服务程序入口地址填入irqaction_t表
	irqaction_t[vector_no] = (intr_handler)handler;
}

/*创建中断门描述符*/
static void create_idt_desc(struct gate_descr* p_gdesc, uint8_t attr, intr_handler intr_func)
{
	p_gdesc->func_offest_low_word = (uint32_t)intr_func & 0x0000FFFF;
	p_gdesc->selector = SELECTOR_K_CODE;
	p_gdesc->dcount = 0;
	p_gdesc->attribute = attr;
	p_gdesc->func_offest_high_word = ((uint32_t)intr_func & 0xFFFF0000) >> 16;
}

/*初始化中断描述符表*/
static void idt_init(void)
{
	int i;
	for(i = 0;i < IDT_DESC_CNT;i++)
	{
		create_idt_desc(&idt[i], IDT_ATTR_DPL0, intr_entry_table[i]);
	}
	/*单独处理系统调用描述符，DPL为3*/
	create_idt_desc(&idt[SYSCALL_NUMBER], IDT_ATTR_DPL3, syscall_handler);
	put_str("    idt initialization completion\n");
}

/*初始化8259A*/
static void pic_8259A_init(void)
{
	//初始化主片
	outb(PIC_M_CTRL, 0x11);
	outb(PIC_M_DATA, 0X20);
	outb(PIC_M_DATA, 0x04);
	outb(PIC_M_DATA, 0x01);

	//初始化从片
	outb(PIC_S_CTRL, 0x11);
	outb(PIC_S_DATA, 0x28);
	outb(PIC_S_DATA, 0x02);
	outb(PIC_S_DATA, 0x01);

	//打开主片IR0，目前只接受时钟中断
	outb(PIC_M_DATA, 0xf8);	//打开定时器，键盘，从片级联中断
	outb(PIC_S_DATA, 0xbf);	//打开从片IRQ14，接受硬盘中断

	put_str("    PIC initialization completion\n");
}

/*开始初始化中断*/
void interrupt_init(void)
{
	put_str("Interrupt initialization start\n");
	idt_init();
	trap_init();
	pic_8259A_init();
	/*加载idt*/
	uint64_t idt_addr = ((sizeof(idt) - 1) | ((uint64_t)(uint32_t)idt << 16));
	asm volatile("lidt %0" : : "m" (idt_addr));
	put_str("Interrupt initialization completion\n");
}


/***********************************************************/
/***************************中断控制*************************/
/**********************************************************/
#define EFLAGS_IF 0x00000200
#define GET_EFLAGS(eflags) do{\
		asm volatile("pushfl;popl %0":"=g"(eflags));\
	}while(0)

/*开中断*/
enum intr_status local_irq_enable(void)
{
	enum intr_status old_status = get_intr_status();
	if(INTR_OFF == old_status)
	{
		asm volatile("sti":::"memory");
	}
	return old_status;
}
/*关中断*/
enum intr_status local_irq_disable(void)
{
	enum intr_status old_status = get_intr_status();
	if(INTR_ON == old_status)
	{
		asm volatile("cli":::"memory");
	}
	return old_status;
}
/*设置中断状态*/
void set_intr_status(enum intr_status status)
{
	(status & INTR_ON) ? local_irq_enable() : local_irq_disable();
}
/*获取中断状态*/
enum intr_status get_intr_status(void)
{
	uint32_t eflags = 0;
	GET_EFLAGS(eflags);
	return (eflags & EFLAGS_IF) ? INTR_ON : INTR_OFF;
}
