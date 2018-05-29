#ifndef __KERNEL_GOLBAL_H
#define __KERNEL_GOLBAL_H
#include "stdint.h"

/*******************/
#define NULL ((void*)0)
#define TRUE 1
#define FALSE 0
#define bool int
#define DIV_ROUND_UP(X, STEP) ((X + STEP - 1) / (STEP))


//------------------选择子定义
#define RPL0    0
#define RPL1    1
#define RPL2    2
#define RPL3    3

#define TI_GDT  0
#define TI_LDT  1

// 内核段选择子
#define SELECTOR_K_CODE     ((1 << 3) + (TI_GDT << 2) + RPL0)
#define SELECTOR_K_DATA     ((2 << 3) + (TI_GDT << 2) + RPL0)
#define SELECTOR_K_STACK    SELECTOR_K_DATA
#define SELECTOR_K_GS       ((3 << 3) + (TI_GDT << 2) + RPL0)
// 用户段选择子
/*第4个段描述符是TSS*/
#define SELECTOR_U_CODE     ((5 << 3) + (TI_GDT << 2) + RPL3)
#define SELECTOR_U_DATA     ((6 << 3) + (TI_GDT << 2) + RPL3)
#define SELECTOR_U_STACK    SELECTOR_U_DATA



// ----------------- GDT描述符属性 ----------
#define GDT_DESC_G_4K   1   /*段界限粒度4KB字节*/
#define GDT_DESC_D_32   1   /*使用EIP寄存器*/
#define GDT_DESC_L      0   /*32位代码*/
#define GDT_DESC_AVL    0
#define GDT_DESC_P	        1
#define GDT_DESC_DPL_0      0   /*段在内存中*/
#define GDT_DESC_DPL_1      1
#define GDT_DESC_DPL_2      2
#define GDT_DESC_DPL_3      3
/********************************************
    代码段和数据段属于存储段，tss及各种门描述符属于
    系统段
    s为1表示存储段，为0表示系统段
********************************************/
#define GDT_DESC_S_CODE     1
#define GDT_DESC_S_DATA     GDT_DESC_S_CODE
#define GDT_DESC_S_SYS      0
#define GDT_DESC_TYPE_CODE  8   /*x=1,c=0,r=0,a=0 代码段可执行，非依从，不可读，已访问位清0*/
#define GDT_DESC_TYPE_DATA  2   /*x=0,e=0,w=1,a=0 数据段不可执行，向上扩展，可写，已访问位清0*/
#define GDT_DESC_TYPE_TSS   9   /*B位0，不忙*/


#define GDT_ATTR_HIGH   ((GDT_DESC_G_4K << 7) + (GDT_DESC_D_32 << 6) + (GDT_DESC_L << 5) + (GDT_DESC_AVL << 4))
#define GDT_CODE_ATTR_LOW_DPL3  ((GDT_DESC_P << 7) + (GDT_DESC_DPL_3 << 5) + (GDT_DESC_S_CODE << 4) + (GDT_DESC_TYPE_CODE))
#define GDT_DATA_ATTR_LOW_DPL3  ((GDT_DESC_P << 7) + (GDT_DESC_DPL_3 << 5) + (GDT_DESC_S_DATA << 4) + (GDT_DESC_TYPE_DATA))

// ---------------- TSS描述符属性 --------------
#define TSS_DESC_D      0
#define TSS_ATTR_HIGH   ((GDT_DESC_G_4K << 7) + (TSS_DESC_D << 6) + (GDT_DESC_L << 5) + (GDT_DESC_AVL << 4) + 0x0)
#define TSS_ATTR_LOW    ((GDT_DESC_P << 7) + (GDT_DESC_DPL_0 << 5) + (GDT_DESC_S_SYS << 4) + (GDT_DESC_TYPE_TSS))

/* TSS选择子 */
#define SELECTOR_TSS    ((4 << 3) + (TI_GDT << 2) + RPL0)


/* GDT描述符结构 */
struct gdt_desc{
    uint16_t limit_low_16;
    uint16_t base_low_16;
    uint8_t base_mid_8;
    uint8_t attr_low_8;
    uint8_t limit_high_attr_high_8;
    uint8_t base_high_8;
};

/*eflags寄存器属性位*/
#define EFLAGS_MBS      (1 << 1)
#define EFLAGS_IF_1     (1 << 9)
#define ELFAGS_IF_0     0
#define ELFAGS_IOPL_3   (3 << 12)
#define EFLAGS_IOPL_0   (0 << 12)

#endif

/*选择子结构

15                        3     2          0
-------------------------------------------
|描述符索引值               | TI |  RPL     |
|                         |    |          |
-------------------------------------------

loader.S中定义GDT顺序:
空  DESC_CODE   DESC_DATA   DESC_VIDEO
*/

/*段描述符格式

31-24         23  22    21  20   19  16  15  14  13 12 11      8  7     0
------------------------------------------------------------------------
|段基址       | G | D/B | L | AVL |段界限 | P | DPL | S |   TYPE  | 段基址 |
|   31-24位  |   |     |   |     |19-16 |   |     | 0 | D 1 1 0 | 23-16 |          高32位
------------------------------------------------------------------------

31                            16 15                                 0
---------------------------------------------------------------------
|           段基址               |        段界限                       |            
|         15-0位                |       15-0位                       |          低32位
---------------------------------------------------------------------

G位表示段界限粒度大小
G位为0表示段粒度大小1字节
G位为1表示段粒度大小4KB字节

D/B指示有效地址及操作数大小
0表示16位，使用IP寄存器；1表示32位，使用EIP寄存器

L设置是否是64位代码段
1表示64位，0表示32位

AVL保留位，硬件没有使用

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