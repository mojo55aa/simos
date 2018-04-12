#ifndef __KERNEL_GOLBAL_H
#define __KERNEL_GOLBAL_H
#include "stdint.h"

//------------------选择子定义
#define RPL0    0
#define RPL1    1
#define RPL2    2
#define RPL3    3

#define TI_GDT  0
#define TI_LDT  1

#define SELECTOR_K_CODE     ((1 << 3) + (TI_GDT << 2) + RPL0)
#define SELECTOR_K_DATA     ((2 << 3) + (TI_GDT << 2) + RPL0)
#define SELECTOR_K_GS       ((3 << 3) + (TI_GDT << 2) + RPL0)


/*******************/
#define NULL ((void*)0)
#define TRUE 1
#define FALSE 0
#define bool int

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