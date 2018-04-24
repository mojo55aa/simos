#ifndef __MM_MEMORY_H
#define __MM_MEMORY_H
#include "stdint.h"
#include "bitmap.h"

/*虚拟地址池结构*/
struct virtual_mem_pool{
    struct bitmap vm_bitmap;    /*虚拟地址池位图*/
    uint32_t vm_start;          /*虚拟地址池字节数量*/
};

#define PAGE_OFFEST 0xc0000000  /*内核线性地址从3GB开始*/

#define __pa(v_addr) ((v_addr) - (PAGE_OFFEST))   /*将3GB以上内核虚拟地址转换成1GB一下物理地址*/
#define __va(p_addr) ((p_addr) + (PAGE_OFFEST))   /*将内核物理地址转换成虚拟地址*/

/*页表的一些属性*/
#define PG_P_ENABLE     1   /*Present存在位，0表示不在内存中*/
#define PG_P_DISABLE    0
#define PG_RW_R         0   /*Read/Write 读写位，0表示可读不可写*/
#define PG_RW_W         2   /*可读可写*/
#define PG_US_S         0   /*User/Supervisor 普通用户、超级用户，0表示超级用户，特权级3程序不能访问*/
#define PG_US_U         4   /*普通用户，任意级别可以访问*/

void mem_init();

#endif