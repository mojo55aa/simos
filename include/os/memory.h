#ifndef __MM_MEMORY_H
#define __MM_MEMORY_H
#include "stdint.h"
#include "bitmap.h"
#include "list.h"

/*虚拟地址池结构*/
struct virtual_mem_pool{
    struct bitmap vm_bitmap;    /*虚拟地址池位图*/
    uint32_t vm_start;          /*虚拟地址池起始地址*/
};

/* 内存块结构 */
struct mem_block{
    struct kernel_list free_block;
};

/* 分配小粒度内存描述符 */
struct mem_block_desc
{
    uint32_t block_size;    /* 内存块大小KB */
    uint32_t blocks_each_arena; /* 一个arena中内存块数量 */
    struct kernel_list free_list;    /* 空闲链表 */
};

#define MEM_DESC_CNT    7   /*16,32,64,128,256,512,1024 7中规格内存块*/


#define PAGE_OFFEST 0xc0000000  /*内核线性地址从3GB开始*/
#define PAGE_SIZE   4096        /*页大小4KB*/
#define PAGE_MASK   0xfffff000
#define PAGE_DIR_TABLE 0x100000                  /*内核页表地址,物理内存1MB处*/

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
enum pool_flags {
    PF_KERNEL = 1, /*内核内存池*/
    PF_USER = 2    /*用户内存池*/
};

//内存初始化
void mem_init();

/* 初始化内存块描述符 */
void mem_block_desc_init(struct mem_block_desc *block_desc_array);

//申请pg_cnt页内存
void *page_alloc(enum pool_flags pf, uint32_t pg_cnt);

//释放cnt页内存
void page_free(enum pool_flags pf, void *vaddr, uint32_t pg_cnt);

/* 在内核堆空间申请页面 */
void *get_kernel_pages(uint32_t pages);

/* 为用户进程申请物理页，返回页表映射的虚拟地址 */
void *get_user_pages(uint32_t pages);

/* 小粒度内存分配 */
void *sys_malloc(uint32_t size);

/* 内存释放 */
void sys_free(void *ptr);

void print_bitmap();

#endif