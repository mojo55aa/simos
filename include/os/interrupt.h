#ifndef __KERNEL_INTERRUPT_H
#define __KERNEL_INTERRUPT_H
#include "stdint.h"

//中断处理函数指针  loader.S中定义
typedef void* intr_handler;

typedef void(*IRQ_HANDLER)();   /*中断服务程序函数指针*/

//中断初始化函数
void interrupt_init(void);
//注册中断服务程序
void register_irq_handler(uint8_t vertor_no, IRQ_HANDLER handler);

//-------------中断描述符IDT属性-----------------
#define IDT_DESC_P          1
#define IDT_DESC_DPL0       0
#define IDT_DESC_DPL3       3
#define IDT_DESC_S          0
#define IDT_DESC_32_TYPE    0xE     //32位中断门
#define IDT_DESC_16_TYPE    0x6     //16位中断门

#define IDT_ATTR_DPL0       ((IDT_DESC_P << 7) + (IDT_DESC_DPL0 << 5) + (IDT_DESC_S << 4) + IDT_DESC_32_TYPE)
#define IDT_ATTR_DPL3       ((IDT_DESC_P << 7) + (IDT_DESC_DPL3 << 5) + (IDT_DESC_S << 4) + IDT_DESC_32_TYPE)


/*开、关中断函数
*这里参考Linux的做法，关中断前保存eflags状态，开中断时恢复
*因为中断处理程序可能对eflags破坏
*/
#define local_irq_save(flags)\
        do{raw_local_irq_save(flags);}while(0)

#define local_irq_restore(flags)\
        do{raw_local_irq_restore(flags);}while(0)

static inline void raw_local_irq_save(uint32_t *f)
{
        asm volatile("pushfl;popl %0":"=g"(f):);
        asm volatile("cli":::"memory");
}

static inline void raw_local_irq_restore(uint32_t f)
{
        asm volatile("sti":::"memory");
        asm volatile("pushl %0;popfl": :"g"(f):"memory","cc");
}



/*开关中断相关定义*/
enum intr_status{       //interrupts on or off
        INTR_OFF,       /*interrupts off*/
        INTR_ON         /*interrupts on*/
};

enum intr_status get_intr_status(void);
void set_intr_status(enum intr_status);
void local_irq_enable(void);
void local_irq_disable(void);





#endif



/*中断门描述符格式

31                            16 15 14  13 12 11      8 7           0
----------------------------------------------------------------------
|中断处理程序在目标代码段偏移量     | P | DPL | S |   TYPE  |   未使用     |
|   31-16位                    |   |     | 0 | D 1 1 0 |             |          高32位
---------------------------------------------------------------------

31                            16 15                                 0
---------------------------------------------------------------------
| 中断处理程序目标代码段描述符      |   中断处理程序在目标代码段偏移量        |            
|       选择子                  |       15-0位                       |          低32位
---------------------------------------------------------------------

P位为0表示中断处理程序不在内存中
P位为1表示……在内存中

D位为0表示16位模式
D位为1表示32位模式

S为0表示系统段（硬件需要用到的代码,门结构CPU会使用）
S为1表示数据段

TYPE:   0101    任务门描述符
        D110    中断门描述符        eflags的IF位自动置0，关中断
        D111    陷阱门描述符        eflags的IF位不自动置0，允许中断嵌套
        D100    调用门描述符

*/
