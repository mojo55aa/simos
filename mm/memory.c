#include "../include/os/memory.h"
#include "../include/os/bitmap.h"
#include "../include/os/debug.h"
#include "../include/os/stdint.h"
#include "../include/asm/print.h"

#define PAGE_SIZE 4094  /*页面大小4KB*/
#define TOTAL_MEM (*(uint32_t*)(0xb00))     /*物理内存容量，loader.S 中total_mem_bytes变量地址0xb00*/

struct physical_mem_pool{
    struct bitmap pool_bitmap;
    uint32_t phy_addr_start;
    uint32_t pool_size;
};

struct physical_mem_pool kernel_pool, user_pool;
struct virtual_mem_pool kernel_vm_pool;


void mem_init()
{
    put_str("memory initialization start\n");
    /*初始化物理内存池位图*/
    
    put_str("memory initialization completion\n");
}