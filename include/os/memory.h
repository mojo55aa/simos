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

void mem_init();

#endif