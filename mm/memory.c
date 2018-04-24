/* ISSUES
 * 内存分配起始地址不是自然页
*/
#include "../include/os/memory.h"
#include "../include/os/bitmap.h"
#include "../include/os/debug.h"
#include "../include/os/stdint.h"
#include "../include/os/string.h"
#include "../include/asm/print.h"

#define PAGE_SIZE 4096  /*页面大小4KB*/
#define ONE_PAGE_DIR_SIZE 0x400000   /*一个页目录项对应4MB内存*/
#define TOTAL_MEM (*(uint32_t*)(0xb00))     /*物理内存容量，loader.S 中total_mem_bytes变量地址0xb00*/

/*内存的一些定义*/
#define KERNEL_STACK_TOP (PAGE_OFFEST+0x9f000)  /*内核栈顶地址,底下一个页框是内核PCB地址0xc009e000*/
#define PAGE_DIR_TABLE  0x100000      /*内核页表地址,物理内存1MB处*/
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
void mem_pool_init(uint64_t total_mem)
{
    /*计算物理内存总页数*/
    //uint32_t total_pages = total_mem / PAGE_SIZE;
    /*根据内存容量，决定内核和用户内存池大小*/
    uint32_t kernel_mem_size;
    uint32_t user_mem_size;
    /*如果内存容量大于2GB，则内核固定使用1GB*/
    if(total_mem >= KERNEL_THRESHOLD)
    {
        kernel_mem_size = KERNEL_MAX_SIZE - (KERNEL_HEAP_BASE - 0xc0000000);
        user_mem_size = total_mem - kernel_mem_size - (KERNEL_HEAP_BASE - 0xc0000000);
    }
    else/*如果小于2GB，内核和用户空间平分*/
    {
        kernel_mem_size = total_mem / 2 - (KERNEL_HEAP_BASE - 0xc0000000);     /*kernel内存池容量需要减去已经分配出去的*/
        user_mem_size = total_mem - kernel_mem_size - (KERNEL_HEAP_BASE - 0xc0000000); /*用户内存池*/
    }

    /*计算kernel和user内存页数*/
    uint32_t kernel_pages = kernel_mem_size / PAGE_SIZE;
    uint32_t user_pages = user_mem_size / PAGE_SIZE;

    /*初始化内存池*/
    kernel_pool.pool_bitmap.bitmap = (void*)MEM_BITMAP_BASE;
    kernel_pool.pool_bitmap.bitmap_bytes_len = kernel_pages / 8;
    //put_hex(kernel_pool.pool_bitmap.bitmap_bytes_len);
    kernel_pool.pool_size = kernel_pages * PAGE_SIZE; /*kernel_mem_size可能不准确*/
    kernel_pool.phy_addr_start = KERNEL_HEAP_BASE - 0xc0000000;   /*可用内存池物理地址从内核堆处开始*/

    user_pool.pool_bitmap.bitmap = (void*)(MEM_BITMAP_BASE + (kernel_pages / 8));
    user_pool.pool_bitmap.bitmap_bytes_len = user_pages / 8;
    user_pool.pool_size = user_pages * PAGE_SIZE;
    user_pool.phy_addr_start = kernel_pages * PAGE_SIZE + (KERNEL_HEAP_BASE - 0xc0000000);
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
    put_hex(kernel_pool.phy_addr_start + kernel_pool.pool_size  - 0x1);

    put_str("\n    kernel vm bitmap address: 0x");
    put_hex((int)kernel_vm_pool.vm_bitmap.bitmap);
    
    put_str("\n    user bitmap address: 0x");
    put_hex((int)user_pool.pool_bitmap.bitmap);
    put_str("\n    user physical address: 0x");
    put_hex(user_pool.phy_addr_start);
    put_str("-0x");
    put_hex(user_pool.phy_addr_start + user_pool.pool_size - 0x1);
    put_char('\n');
}

/**
 * get_kernel_pages 返回内核空间页数
 * @total_mem: 总内存数
 * return: 内核页表数
*/
static inline uint32_t get_kernel_pages(uint64_t total_mem)
{
    uint32_t kernel_mem_size;
    /*如果内存容量大于2GB，则内核固定使用1GB*/
    if(total_mem >= KERNEL_THRESHOLD)
    {
        kernel_mem_size = KERNEL_MAX_SIZE;
    }
    else/*如果小于2GB，内核和用户空间平分*/
    {
        kernel_mem_size = total_mem / 2; /*kernel内存池容量需要减去已经分配出去的*/
    }

    /*计算kernel和user内存页数*/
    return (kernel_mem_size / PAGE_SIZE);
}
/**
 * page_table_init 初始化内核页表，对内核空间做完整映射，替换临时页表。
 * 页目录中前768项不做映射，内核地址从0xc0000000开始
 * 0xc0000000-0xcfffffff:0x00000000-0x3fffffff
 * 最后一个页表指向页目录表，这样kernel最高4MB没有映射
 * @kernel_pages: 内核页数
*/
void page_table_init(uint32_t kernel_pages)
{
    /*页目录表的地址*/
    uint32_t *page_dir_addr = (uint32_t *)PAGE_DIR_TABLE;
    /*计算需要的页目录数和最后一个页目录对应页表项数*/
    uint32_t page_dir_num, last_page_table_num;
    if ((kernel_pages * PAGE_SIZE) % ONE_PAGE_DIR_SIZE == 0) /*总内存字节数除以4MB,余数为0*/
    {
        page_dir_num = (kernel_pages * PAGE_SIZE) / ONE_PAGE_DIR_SIZE;
        last_page_table_num = 0;
    }
    else    /*余数不为0，需要计算最后一个页目录项对应的页表项数*/
    {
        page_dir_num = (kernel_pages * PAGE_SIZE) / ONE_PAGE_DIR_SIZE;
        /*（总内存字节数-已分配的完整内存字节数） / 一页大小*/
        last_page_table_num = ((kernel_pages * PAGE_SIZE) - (ONE_PAGE_DIR_SIZE * (page_dir_num))) / PAGE_SIZE;
    }

    /*初始化页目录表*/
    uint16_t kernel_page_mark = 0x0 | PG_P_ENABLE | PG_RW_W | PG_US_U; /*kernel页表属性，存在 可写 普通用户*/
    uint32_t i;

    memset((void*)PAGE_DIR_TABLE, 0, 1024 * 1024);   /*页表占用的空间清零*/
    // for (i = 0; i < 768; i++) /*前768项，3GB空间不映射*/
    // {
    //     page_dir_addr[i] = 0x0;
    // }

    uint32_t page_table_base = (uint32_t)page_dir_addr + 0x1000;  /*内核页表基址，页目录表的下一个页框*/
    for (i = 768; i < 768 + page_dir_num; i++) /*映射内核空间*/
    {
        /*每一个页目录项*/
        uint32_t each_page_dir = page_table_base + ((i - 768) << 12) + kernel_page_mark;
        page_dir_addr[i] = each_page_dir;
    }

    /*初始化页表*/
    /*每次迭代的物理内存地址*/
    uint32_t memory_addr;
    for (i = 0; i < page_dir_num; i++)
    {
        memory_addr = i * ONE_PAGE_DIR_SIZE;
        /*每次迭代的页表地址*/
        uint32_t *page_table_addr = (uint32_t*)(page_table_base + (i << 12));
        int j;
        for (j = 0; j < 1024; j++)
        {
            /*构造页表项*/
            //从物理地址0x0开始
            uint32_t each_page_table = (memory_addr + j * PAGE_SIZE) + kernel_page_mark;
            page_table_addr[j] = each_page_table;
        }    
    }

    /*对不是整数的页目录项单独赋值*/
    if (last_page_table_num)
    {
        memory_addr += ONE_PAGE_DIR_SIZE;    /*调整物理内存地址，加4MB*/
        for (i = 0; i < last_page_table_num; i++)
        {
            uint32_t each_page_table = (memory_addr + i * PAGE_SIZE) + kernel_page_mark;
            /*计算页表的地址*/
            *(uint32_t*)((page_table_base + (page_dir_num + 1) * 0x1000) + i * 4) = each_page_table;
        }
        page_dir_addr[768 + page_dir_num] = (page_table_base + (page_dir_num + 1) * 0x1000) + kernel_page_mark;
    }
    /*让最后一个页目录项指向页目录表的基址*/
    page_dir_addr[1023] = (uint32_t)page_dir_addr + kernel_page_mark;
    /**************************************************/
    /*重新加载页目录表*/
    asm volatile("movl %0, %%cr3": :"r"(page_dir_addr));
}

void mem_init()
{
    put_str("memory initialization start\n");
    /*初始化内存池*/
    mem_pool_init((uint64_t)TOTAL_MEM);
    /*初始化页表*/
    uint32_t kernel_pages = get_kernel_pages((uint64_t)TOTAL_MEM);
    page_table_init(kernel_pages);
    put_str("    kernel PDE and PTE reload completion\n");
    put_str("memory initialization completion\n");
}
