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
#define PAGE_SIZE   4096        /*页大小4KB*/

#define __pa(v_addr) ((v_addr) - (PAGE_OFFEST))   /*将3GB以上内核虚拟地址转换成1GB一下物理地址*/
#define __va(p_addr) ((p_addr) + (PAGE_OFFEST))   /*将内核物理地址转换成虚拟地址*/

/*页表的一些属性*/
#define PG_P_ENABLE     1   /*Present存在位，0表示不在内存中*/
#define PG_P_DISABLE    0
#define PG_RW_R         0   /*Read/Write 读写位，0表示可读不可写*/
#define PG_RW_W         2   /*可读可写*/
#define PG_US_S         0   /*User/Supervisor 普通用户、超级用户，0表示超级用户，特权级3程序不能访问*/
#define PG_US_U         4   /*普通用户，任意级别可以访问*/

/*内存池标记*/
enum pool_flags
{
    PF_KERNEL = 1,  /*内核内存池*/
    PF_USER = 2     /*用户内存池*/
};

//内存初始化
void mem_init();

//申请pg_cnt页内存
void *page_alloc(enum pool_flags pf, uint32_t pg_cnt);

/*在内核堆空间申请页面*/
void *get_kernel_pages(uint32_t pages);

/*为用户进程申请物理页，返回页表映射的虚拟地址*/
void *get_user_pages(uint32_t pages);

#endif