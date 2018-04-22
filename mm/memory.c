/* ISSUES
 * 内存分配起始地址不是自然页
*/
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
#define KERNEL_VM_BITMAP_BASE 0xc0075000    /*内核虚拟地址池最大1GB，需要8个页框的位图*/

#define KERNEL_HEAP_BASE 0xc1000000     /*KERNEL堆空间从16MB处开始，以后内核和页表可能会移动到1MB之上*/

#define KERNEL_THRESHOLD 0x80000000     /*物理内存阈值，大于2GB时，kernel固定1GB*/
#define KERNEL_MAX_SIZE 0x40000000      /*kernel空间最大值*/

/*后期内核移动到16MB处，0x1000000，低端1MB内存不动*/


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


/**
 * mem_pool_init -内存池初始化
 * @total_mem: 物理内存总字节数
 * 根据物理内存总字节数，计算页数，分配内核物理内存池和用户物理内存池
*/
void mem_pool_init(uint32_t total_mem)
{
    /*计算物理内存总页数*/
    uint32_t total_pages = total_mem / PAGE_SIZE;
    /*根据内存容量，决定内核和用户内存池大小*/
    uint32_t kernel_mem_size;
    uint32_t user_mem_size;
    /*如果内存容量大于2GB，则内核固定使用1GB*/
    if(total_mem >= KERNEL_THRESHOLD)
    {
        kernel_mem_size = KERNEL_MAX_SIZE;
        user_mem_size = total_mem - kernel_mem_size;
    }
    else/*如果小于2GB，内核和用户空间平分*/
    {
        kernel_mem_size = total_mem / 2;
        user_mem_size = total_mem - kernel_mem_size;
    }

    /*计算kernel和user内存页数*/
    uint32_t kernel_pages = kernel_mem_size / PAGE_SIZE;
    uint32_t user_pages = user_mem_size / PAGE_SIZE;
    /*初始化内存池*/
    kernel_pool.pool_bitmap.bitmap = (void*)MEM_BITMAP_BASE;
    kernel_pool.pool_bitmap.bitmap_bytes_len = kernel_pages / 8;

    put_hex(kernel_pool.pool_bitmap.bitmap_bytes_len);
    put_char('\n');
    kernel_pool.pool_size = kernel_pages * PAGE_SIZE; /*kernel_mem_size可能不准确*/
    kernel_pool.phy_addr_start = 0x0;   /*内核空间从起始地址完整映射*/
    user_pool.pool_bitmap.bitmap = (void*)(MEM_BITMAP_BASE + (kernel_pages / 8));
    user_pool.pool_bitmap.bitmap_bytes_len = user_pages / 8;
    user_pool.pool_size = user_pages * PAGE_SIZE;
    user_pool.phy_addr_start = kernel_pages * PAGE_SIZE + 1;
    /*将位图初始化*/
    bitmap_zero(&kernel_pool.pool_bitmap);
    bitmap_zero(&user_pool.pool_bitmap);
    /*初始化kernel虚拟地址池*/
    kernel_vm_pool.vm_bitmap.bitmap_bytes_len = kernel_pages / 8;
    kernel_vm_pool.vm_bitmap.bitmap = (void *)KERNEL_VM_BITMAP_BASE;
    kernel_vm_pool.vm_start = KERNEL_HEAP_BASE;

    bitmap_zero(&kernel_vm_pool.vm_bitmap);
    put_str("    kernel bitmap address: 0x");
    put_hex((int)kernel_pool.pool_bitmap.bitmap);
    put_str("\n    kernel physical address: 0x");
    put_hex(kernel_pool.phy_addr_start);
    put_str("-0x");
    put_hex(kernel_pool.phy_addr_start + kernel_pool.pool_size);

    put_str("\n    kernel vm bitmap address: 0x");
    put_hex((int)kernel_vm_pool.vm_bitmap.bitmap);
    
    put_str("\n    user bitmap address: 0x");
    put_hex((int)user_pool.pool_bitmap.bitmap);
    put_str("\n    user physical address: 0x");
    put_hex(user_pool.phy_addr_start);
    put_str("-0x");
    put_hex(user_pool.phy_addr_start + user_pool.pool_size);
    put_char('\n');
}

void mem_init()
{
    put_str("memory initialization start\n");
    /*初始化内存池*/
    mem_pool_init((uint32_t)0x2000000);
    put_str("memory initialization completion\n");
}