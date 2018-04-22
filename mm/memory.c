#include "../include/os/memory.h"
#include "../include/os/bitmap.h"
#include "../include/os/debug.h"
#include "../include/os/stdint.h"
#include "../include/asm/print.h"

#define PAGE_SIZE 4094  /*页面大小4KB*/
#define TOTAL_MEM (*(uint32_t*)(0xb00))     /*物理内存容量，loader.S 中total_mem_bytes变量地址0xb00*/

/*内存的一些定义*/
#define KERNEL_STACK_TOP (PAGE_OFFEST+0x9f000)  /*内核栈顶地址,底下一个页框是内核PCB地址0xc009e000*/
#define PAGE_DIR_TABLE  0xc0010000      /*内核页表地址*/
#define MEM_BITMAP_BASE 0xc007e000      /*位图地址,kernel PCB位于0xc009e000处，预留32个页框，最大支持4GB内存*/

#define KERNEL_HEAP_BASE 0xc1000000     /*KERNEL堆空间从16MB处开始，以后内核和页表可能会移动到1MB之上*/

/**
 * 物理地址池结构
 * 根据TOTAL_MEM值，动态划分
*/
struct physical_mem_pool{
    struct bitmap pool_bitmap;  /*物理内存池位图*/
    uint32_t phy_addr_start;    /*内存池物理内存起始地址*/
    uint32_t pool_size;     /*内存池字节容量最大4GB*/
                            /*内核1GB，用户进程3GB*/
};


struct physical_mem_pool kernel_pool, user_pool;    /*kernel,user process物理地址池*/
struct virtual_mem_pool kernel_vm_pool;         /*KERNEL虚拟地址池*/


void mem_pool_init(uint32_t total_mem)
{
    
}


void mem_init()
{
    put_str("memory initialization start\n");
    /*初始化物理内存池位图*/

    put_str("memory initialization completion\n");
}