/* ISSUES
 * 内存分配起始地址不是自然页
 * 创建页表过程可能还有问题，第一个页表第一项应该是0x0007   :::resolved
*/
#include "../include/os/memory.h"
#include "../include/os/bitmap.h"
#include "../include/os/debug.h"
#include "../include/os/stdint.h"
#include "../include/os/string.h"
#include "../include/asm/print.h"
#include "../include/os/sync.h"
#include "../include/os/thread.h"
#include "../include/os/interrupt.h"

#define ONE_PAGE_DIR_SIZE 0x400000       /*一个页目录项对应4MB内存*/
#define TOTAL_MEM (*(uint32_t *)(0xb00)) /*物理内存容量，loader.S 中total_mem_bytes变量地址0xb00*/

/*内存的一些定义*/
#define KERNEL_STACK_TOP (PAGE_OFFEST + 0x9f000) /*内核栈顶地址,底下一个页框是内核PCB地址0xc009e000*/
#define MEM_BITMAP_BASE 0xc007e000               /*位图地址,kernel PCB位于0xc009e000处，预留32个页框，最大支持4GB内存*/
#define KERNEL_VM_BITMAP_BASE 0xc0075000         /*内核虚拟地址池最大1GB，需要8个页框的位图*/

#define KERNEL_HEAP_BASE 0xc1000000 /*KERNEL堆空间从16MB处开始，以后内核和页表可能会移动到1MB之上*/

#define KERNEL_THRESHOLD 0x80000000 /*物理内存阈值，大于2GB时，kernel固定1GB*/
#define KERNEL_MAX_SIZE 0x40000000  /*kernel空间最大值*/

/*后期内核移动到16MB处，0x1000000，低端1MB内存不动*/

/**
 * 物理地址池结构
 * 根据TOTAL_MEM值，动态划分
*/
struct physical_mem_pool
{
    struct bitmap pool_bitmap; /*物理内存池位图*/
    uint32_t phy_addr_start;   /*内存池物理内存起始地址*/
    uint32_t pool_size;        /*内存池字节容量最大4GB*/
                               /*内核1GB，用户进程3GB*/
    struct lock lock;          /*申请内存池时的锁*/
};

/* 内存仓库元信息 */
struct arena{
    struct mem_block_desc *desc;
    uint32_t cnt;   /* 空闲内存块数或者页框数 */
};
/* 内存块描述符数组 */
struct mem_block_desc kernel_block_desc_array[MEM_DESC_CNT];


struct physical_mem_pool kernel_pool, user_pool; /*kernel,user process物理地址池*/
struct virtual_mem_pool kernel_vm_pool;          /*KERNEL虚拟地址池*/

static void *vaddr_alloc(enum pool_flags pf, uint32_t pg_cnt);
static void *phy_alloc(struct physical_mem_pool *phy_pool);
static void vaddr_free(enum pool_flags pf, void *vaddr, uint32_t pg_cnt);
static void phy_free(uint32_t phy_addr);
static void page_table_init(uint32_t kernel_pages);

/**
 * mem_block_desc_init --初始化所有内存块描述符
 * @block_array: 内存块描述符数组
*/
void mem_block_desc_init(struct mem_block_desc* block_desc_array)
{
    uint8_t desc_idx = 0;
    uint16_t block_size = 16;
    for (desc_idx = 0; desc_idx < MEM_DESC_CNT; desc_idx++)
    {
        block_desc_array[desc_idx].block_size = block_size;
        block_desc_array[desc_idx].blocks_each_arena = (PAGE_SIZE - sizeof(struct arena)) / block_size;
        list_init(&block_desc_array[desc_idx].free_list);
        block_size *= 2;
    }
}

/**
 * mem_pool_init -内存池初始化
 * @total_mem: 物理内存总字节数
 * 根据物理内存总字节数，计算页数，分配内核物理内存池和用户物理内存池
*/
static void
mem_pool_init(uint64_t total_mem)
{
    /*计算物理内存总页数*/
    //uint32_t total_pages = total_mem / PAGE_SIZE;
    /*根据内存容量，决定内核和用户内存池大小*/
    uint32_t kernel_mem_size;
    uint32_t user_mem_size;
    /*如果内存容量大于2GB，则内核固定使用1GB*/
    if (total_mem >= KERNEL_THRESHOLD)
    {
        kernel_mem_size = KERNEL_MAX_SIZE - (KERNEL_HEAP_BASE - 0xc0000000);
        user_mem_size = total_mem - kernel_mem_size - (KERNEL_HEAP_BASE - 0xc0000000);
    }
    else /*如果小于2GB，内核和用户空间平分*/
    {
        kernel_mem_size = total_mem / 2 - (KERNEL_HEAP_BASE - 0xc0000000);             /*kernel内存池容量需要减去已经分配出去的*/
        user_mem_size = total_mem - kernel_mem_size - (KERNEL_HEAP_BASE - 0xc0000000); /*用户内存池*/
    }

    /*计算kernel和user内存页数*/
    uint32_t kernel_pages = kernel_mem_size / PAGE_SIZE;
    uint32_t user_pages = user_mem_size / PAGE_SIZE;

    /*初始化内存池*/
    kernel_pool.pool_bitmap.bitmap = (void *)MEM_BITMAP_BASE;
    kernel_pool.pool_bitmap.bitmap_bytes_len = kernel_pages / 8;
    //put_hex(kernel_pool.pool_bitmap.bitmap_bytes_len);
    kernel_pool.pool_size = kernel_pages * PAGE_SIZE;           /*kernel_mem_size可能不准确*/
    kernel_pool.phy_addr_start = KERNEL_HEAP_BASE - 0xc0000000; /*可用内存池物理地址从内核堆处开始*/

    user_pool.pool_bitmap.bitmap = (void *)(MEM_BITMAP_BASE + (kernel_pages / 8));
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

    /*初始化锁*/
    lock_init(&kernel_pool.lock);
    lock_init(&user_pool.lock);

    put_str("    kernel bitmap address: 0x");
    put_hex((int)kernel_pool.pool_bitmap.bitmap);
    put_str("\n    kernel physical address: 0x");
    put_hex(kernel_pool.phy_addr_start);
    put_str("-0x");
    put_hex(kernel_pool.phy_addr_start + kernel_pool.pool_size - 0x1);

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
 * page_alloc --在指定内存池中申请连续的n页内存，完成虚拟地址物理地址映射，
 * 完成页表的更新,成功返回起始虚拟地址，失败返回NULL
 * 
 * (1) 通过vaddr_alloc在地址池中申请虚拟地址
 * (2) 通过phy_alloc在内存池中申请物理地址
 * (3) 在页表中完成虚拟地址与物理地址的映射
 * 
 * @pf: 内存池标记
 * @pg_cnt: 申请的页数
 * return: void* or NULL
*/
void *page_alloc(enum pool_flags pf, uint32_t pg_cnt)
{
    /*在这里判断什么内存池*/
    if (pf == PF_KERNEL)
    {
        /*先在虚拟地址池中申请*/
        uint32_t *vm_addr = (uint32_t *)vaddr_alloc(PF_KERNEL, pg_cnt);
        if (vm_addr == NULL)
            return NULL;
        /*kernel线性映射，所以直接在物理内存池位图中将相应位置位*/
        uint32_t phy_addr = __pa((uint32_t)vm_addr);                            /*将内核虚拟地址转换成物理地址*/
        uint32_t bit_idx = (phy_addr - kernel_pool.phy_addr_start) / PAGE_SIZE; /*计算位数*/
        bitmap_set_bits(&kernel_pool.pool_bitmap, bit_idx, pg_cnt, bit_set);
        return (void *)vm_addr;
    }
    /*用户内存池*/
    /*先在用户地址池中申请虚拟地址*/
    uint32_t *vm_addr = (uint32_t *)vaddr_alloc(PF_USER, pg_cnt);
    if (vm_addr == NULL)
    {
        return NULL;
    }
    uint32_t vm_addr_tmp = (uint32_t)vm_addr;
    while (pg_cnt-- > 0)
    {
        /*在用户内存池中申请物理地址*/
        uint32_t *phy_addr = (uint32_t*)phy_alloc(&user_pool);
        if (phy_addr == NULL)
        {   //TODO 失败时要完成回滚操作，以后内存回收时再补充
            return NULL;
        }
        //TODO页表映射部分
        /*页表映射*/
        vm_addr_tmp += PAGE_SIZE;
    }
    return (void *)vm_addr;
}

/**
 * page_free --释放虚拟地址vaddr映射的物理内存
 * 
 * (1) 清除物理内存位图中的相应位
 * (2) 清除虚拟地址池位图中的相应位
 * (3) 清除页表映射关系
 * @pf: 内存池标记
 * @vaddr: 待释放内存的虚拟地址
 * @pg_cnt: 释放的页数
*/
void page_free(enum pool_flags pf, void* vaddr, uint32_t pg_cnt)
{
    uint32_t phy_addr;
    uint32_t __vaddr = (uint32_t)vaddr;
    uint32_t pg_idx = 0;
    ASSERT(pg_cnt >= 1);
    /* 是物理内存池 */
    if(pf == PF_KERNEL)
    {
        phy_addr = __pa(__vaddr);
        /* 释放物理内存 */
        while(pg_idx < pg_cnt)
        {
            phy_free(phy_addr);
            phy_addr += PAGE_SIZE;
            pg_idx++;
        }
        /* 释放虚拟地址 */
        vaddr_free(PF_KERNEL, vaddr, pg_cnt);
    }
    else    /* 是用户内存池 */
    {
        /* 释放物理内存，清除页表映射关系 */
        while(pg_idx < pg_cnt)
        {
            //TODO 根据虚拟地址计算物理地址
            phy_addr = __vaddr;
            phy_free(phy_addr);
            //TODO 将页表映射关系清除
            __vaddr += PAGE_SIZE;
            pg_idx++;
        }
        /* 释放虚拟地址 */
        vaddr_free(PF_USER, vaddr, pg_cnt);
    }
}

/**
 * total_kernel_pages 返回内核空间页数
 * @total_mem: 总内存数
 * return: 内核页表数
*/
static inline uint32_t total_kernel_pages(uint64_t total_mem)
{
    uint32_t kernel_mem_size;
    /*如果内存容量大于2GB，则内核固定使用1GB*/
    if (total_mem >= KERNEL_THRESHOLD)
    {
        kernel_mem_size = KERNEL_MAX_SIZE;
    }
    else /*如果小于2GB，内核和用户空间平分*/
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
static void page_table_init(uint32_t kernel_pages)
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
    else /*余数不为0，需要计算最后一个页目录项对应的页表项数*/
    {
        page_dir_num = (kernel_pages * PAGE_SIZE) / ONE_PAGE_DIR_SIZE;
        /*（总内存字节数-已分配的完整内存字节数） / 一页大小*/
        last_page_table_num = ((kernel_pages * PAGE_SIZE) - (ONE_PAGE_DIR_SIZE * (page_dir_num))) / PAGE_SIZE;
    }

    /*初始化页目录表*/
    uint16_t kernel_page_mark = 0x0 | PG_P_ENABLE | PG_RW_W | PG_US_U; /*kernel页表属性，存在 可写 普通用户*/
    uint32_t i;

    memset((void *)(uint32_t)PAGE_DIR_TABLE, 0, 1024 * 1024); /*页表占用的空间清零*/
    // for (i = 0; i < 768; i++) /*前768项，3GB空间不映射*/
    // {
    //     page_dir_addr[i] = 0x0;
    // }

    uint32_t page_table_base = (uint32_t)page_dir_addr + 0x1000; /*内核页表基址，页目录表的下一个页框*/

    //page_dir_addr[0] = ((uint32_t)page_dir_addr + 0x1000) + kernel_page_mark;  /*第一个页目录项，映射0x0~0x100000  --  0x0~0x100000*/

    for (i = 768; i < 768 + page_dir_num; i++) /*映射内核空间*/
    {
        /*每一个页目录项*/
        uint32_t each_page_dir = page_table_base + ((i - 768) << 12) + kernel_page_mark;
        page_dir_addr[i] = each_page_dir;
    }

    /*初始化低端1MB内存对应的页表*/
    // memset((void*)((uint32_t)page_dir_addr + 0x1000), 0, 4 * 1024);
    // for (i = 0; i < 1024; i++)
    // {
    //     *(uint32_t*)((uint32_t)page_dir_addr + 0x1000 + 4 * i) = i * PAGE_SIZE + kernel_page_mark;
    // }

    /*初始化3GB以上页表*/
    /*每次迭代的物理内存地址*/
    uint32_t memory_addr = 0;
    for (i = 0; i < page_dir_num; i++)
    {
        memory_addr = i * ONE_PAGE_DIR_SIZE;
        /*每次迭代的页表地址*/
        uint32_t *page_table_addr = (uint32_t *)(page_table_base + (i << 12));
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
        memory_addr += ONE_PAGE_DIR_SIZE; /*调整物理内存地址，加4MB*/
        for (i = 0; i < last_page_table_num; i++)
        {
            uint32_t each_page_table = (memory_addr + i * PAGE_SIZE) + kernel_page_mark;
            /*计算页表的地址*/
            *(uint32_t *)((page_table_base + (page_dir_num + 1) * 0x1000) + i * 4) = each_page_table;
        }
        page_dir_addr[768 + page_dir_num] = (page_table_base + (page_dir_num + 1) * 0x1000) + kernel_page_mark;
    }
    /*让最后一个页目录项指向页目录表的基址*/
    page_dir_addr[1023] = (uint32_t)page_dir_addr + kernel_page_mark;
    /**************************************************/
    /*重新加载页目录表*/
    asm volatile("movl %0, %%cr3"
                 :
                 : "r"(page_dir_addr));
}

/**
 * vaddr_alloc --在pf表示的地址池中申请pg_cnt个连续页
 * 成功返回虚拟页的起始地址，失败返回NULL
 * @pf: 地址池类型
 * @pg_cnt: 申请的页数量
*/
static void *vaddr_alloc(enum pool_flags pf, uint32_t pg_cnt)
{
    ASSERT(pf == PF_KERNEL || pf == PF_USER);
    int32_t vaddr_start = 0;
    int32_t bit_idx_start = -1;
    if (pf == PF_KERNEL)
    {
        bit_idx_start = bitmap_apply(&kernel_vm_pool.vm_bitmap, pg_cnt);
        if (bit_idx_start == -1)
            return NULL;
        /*将位图对应位置位*/
        uint32_t i = 0;
        while (i < pg_cnt)
        {
            bitmap_set_bit(&kernel_vm_pool.vm_bitmap, bit_idx_start + i++, bit_set);
        }
        vaddr_start = kernel_vm_pool.vm_start + bit_idx_start * PAGE_SIZE;
    }
    else /*用户内存池，以后实现*/
    {
        struct task_struct *cur_task = get_cur_task();
        bit_idx_start = bitmap_apply(&cur_task->userprog_vaddr.vm_bitmap, pg_cnt);
        if (bit_idx_start == -1)
        {
            return NULL;
        }
        //对应位图置位
        bitmap_set_bits(&cur_task->userprog_vaddr.vm_bitmap, bit_idx_start, pg_cnt, bit_set);
        vaddr_start = cur_task->userprog_vaddr.vm_start + bit_idx_start * PAGE_SIZE;
        ASSERT((uint32_t)vaddr_start < (0xc0000000 - PAGE_SIZE));
    }
    return (void *)vaddr_start;
}

/**
 * phy_alloc --在phy_pool指向的物理内存池中分配一个页面
 * @phy_pool: 物理内存池
 * return: 成功返回页框的物理地址，失败返回NULL
*/
static void *phy_alloc(struct physical_mem_pool *phy_pool)
{
    ASSERT(phy_pool != NULL);
    int32_t bit_idx = bitmap_apply(&phy_pool->pool_bitmap, 1);
    if (bit_idx == -1)
        return NULL;
    bitmap_set_bit(&phy_pool->pool_bitmap, bit_idx, bit_set);
    uint32_t page_phyaddr = bit_idx * PAGE_SIZE + phy_pool->phy_addr_start;
    return (void *)page_phyaddr;
}

/**
 * phy_free --在物理内存池中释放一页内存
 * 将相应的位图位置0
 * @phy_addr: 释放的物理地址
*/
static void phy_free(uint32_t phy_addr)
{
    struct physical_mem_pool *mem_pool;
    uint32_t bit_idx = 0;
    /* 用户内存池 */
    if (phy_addr >= user_pool.phy_addr_start)
    {
        mem_pool = &user_pool;
        bit_idx = (phy_addr - user_pool.phy_addr_start) / PAGE_SIZE;
    }
    else    /* 内核内存池 */
    {
        mem_pool = &kernel_pool;
        bit_idx = (phy_addr - kernel_pool.phy_addr_start) / PAGE_SIZE;
    }
    bitmap_set_bit(&mem_pool->pool_bitmap, bit_idx, bit_clear);
}

/**
 * vaddr_free --在虚拟地址池中释放内存
 * 判断kernel还是用户地址池，相应位图位置0
 * @pf: 标记是kernel还是用户地址池
 * @vaddr: 释放内存起始虚拟地址
 * @pg_cnt: 释放的页数
*/
static void vaddr_free(enum pool_flags pf, void* vaddr, uint32_t pg_cnt)
{
    uint32_t __vaddr = (uint32_t)vaddr;
    uint32_t bit_idx = 0;
    if(pf == PF_KERNEL)
    {
        bit_idx = (__vaddr - kernel_vm_pool.vm_start) / PAGE_SIZE;
        bitmap_set_bits(&kernel_vm_pool.vm_bitmap, bit_idx, pg_cnt, bit_clear);
    }
    else
    {
        struct task_struct *cur_task = get_cur_task();
        bit_idx = (__vaddr - cur_task->userprog_vaddr.vm_start) / PAGE_SIZE;
        bitmap_set_bits(&cur_task->userprog_vaddr.vm_bitmap, bit_idx, pg_cnt, bit_clear);
    }
}

/**
 * get_kernel_pages --在内核堆空间申请物理页面
 * @pages: 申请的物理页数
 * return: 成功返回页面起始虚拟地址，失败返回NULL
*/
void *get_kernel_pages(uint32_t pages)
{
    /*对kernel内存池加锁*/
    lock_acquire(&kernel_pool.lock);
    /*得到虚拟地址，已经完成地址池，内存池同步，页表更新*/
    void *vaddr = page_alloc(PF_KERNEL, pages);
    /*对返回的内存区域清零*/
    if (vaddr != NULL)
    {
        memset(vaddr, 0, pages * PAGE_SIZE);
    }
    lock_release(&kernel_pool.lock);
    return vaddr;
}

/**
 * get_user_pages --在用户内存空间申请物理页面
 * @pages: 申请的物理页数
 * return: 成功返回页面起始虚拟地址，失败返回NULL
*/
void *get_user_pages(uint32_t pages)
{
    lock_acquire(&user_pool.lock);
    /*返回虚拟地址，已经完成页表的更新，地址池与内存池的同步*/
    void *vaddr = page_alloc(PF_USER, pages);
    if (vaddr != NULL)
    {
        memset(vaddr, 0, pages * PAGE_SIZE);
    }
    lock_release(&user_pool.lock);
    return vaddr;
}


/**************************** 细粒度内存分配 ********************************/

/**
 * arena_2_block --返回arena中第idx个内存块的地址
 * @arena: arena结构
 * @idx: 索引
*/
static struct mem_block* arena_2_block(struct arena* arena, uint32_t idx)
{
    return (struct mem_block *)((uint32_t)arena + sizeof(struct arena) + idx * arena->desc->block_size);
}

/**
 * block_2_arena --返回内存块所在的arena地址
 * @block: 内存块地址
*/
static struct arena* block_2_arena(struct mem_block* block)
{
    return (struct arena *)((uint32_t)block & PAGE_MASK);
}

/**
 * sys_malloc --在堆中分配size字节内存
 * @size: 申请的内存字节数
 * return: 内存首地址
*/
void* sys_malloc(uint32_t size)
{
    enum pool_flags PF; /* 内存池类型 */
    struct physical_mem_pool *mem_pool; /* 指向用户物理内存池或内核物理内存池 */
    uint32_t pool_size; /* 内存池容量 */
    struct mem_block_desc *block_desc;  /* 内存块描述符类型 */

    /* 判断是内核进程还是用户进程 */
    struct task_struct *cur_task = get_cur_task();
    if (cur_task->pde_addr == NULL)
    {
        PF = PF_KERNEL;
        mem_pool = &kernel_pool;
        pool_size = kernel_pool.pool_size;
        block_desc = kernel_block_desc_array;
    }
    else
    {
        PF = PF_USER;
        mem_pool = &user_pool;
        pool_size = user_pool.pool_size;
        block_desc = cur_task->user_block_desc_array;
    }

    /* 判断申请的内存容量是否合法 */
    if(size > pool_size || size <= 0)
    {
        return NULL;
    }

    struct arena *__arena;
    struct mem_block *__block;
    /* 内存池加锁 */
    lock_acquire(&mem_pool->lock);

    /* size > 1024,分配整个页框 */
    if(size > 1024)
    {
        /* 计算需要几页内存 */
        uint32_t page_cnt = DIV_ROUND_UP(size + sizeof(struct arena), PAGE_SIZE);
        /* 申请整页内存 */
        __arena = (struct arena*)page_alloc(PF, page_cnt);
        if(__arena == NULL)
        {
            lock_release(&mem_pool->lock);
            return NULL;
        }
        /* 准备返回内存 */
        memset(__arena, 0, page_cnt * PAGE_SIZE);
        __arena->desc = NULL;
        __arena->cnt = page_cnt;
        lock_release(&mem_pool->lock);
        /* 返回—__arena开始的地址 */
        return (void *)(__arena + 1);
    }
    
    /* size < 1024,找到使用的内存块描述符 */
    uint8_t desc_idx = 0;
    for (desc_idx = 0; desc_idx < MEM_DESC_CNT;desc_idx++)
    {
        if(size <= block_desc[desc_idx].block_size)
            break;
    }

    /* 如果空闲链表空，则申请一页框 */
    if(list_empty(&block_desc[desc_idx].free_list))
    {
        __arena = (struct arena*)page_alloc(PF, 1);
        if (__arena == NULL)
        {
            lock_release(&mem_pool->lock);
            return NULL;
        }
        memset(__arena, 0, PAGE_SIZE);
        __arena->desc = &block_desc[desc_idx];
        __arena->cnt = block_desc[desc_idx].blocks_each_arena;
        /* 将内存块划分，添加到free_list */
        uint32_t block_idx = 0;
        enum intr_status old_intr = local_irq_disable();
        for (block_idx = 0; block_idx < block_desc[desc_idx].blocks_each_arena; block_idx++)
        {
            __block = arena_2_block(__arena, block_idx);
            ASSERT(!list_find_item(&__arena->desc->free_list, &__block->free_block));
            /* 按序插入 */
            // put_hex(list_len(&__arena->desc->free_list));
            // ASSERT(list_find_item(&__arena->desc->free_list, &__block->free_block));
            list_add_prev(&__arena->desc->free_list, &__block->free_block);
        }
        set_intr_status(old_intr);
    }

    /* 开始分配内存 */
    __block = list_entry(list_pop(&block_desc[desc_idx].free_list), struct mem_block, free_block);
    memset(__block, 0, block_desc[desc_idx].block_size);
    __arena = block_2_arena(__block);
    __arena->cnt--;
    lock_release(&mem_pool->lock);
    return (void *)__block;
}

/**
 * sys_free --释放内存
 * (1) 将ptr转换为mem_block,获取对应元信息arena
 * (2) 将内存块加入对应规格内存块空闲链表
 * (3) 更新元信息
 * (4) 如果整个arena都空闲，则释放物理页
 * @ptr: 释放内存的虚拟地址
*/
void sys_free(void* ptr)
{
    ASSERT(ptr != NULL);
    if (ptr == NULL)
    {
        return;
    }
    enum pool_flags pf;
    struct physical_mem_pool *mem_pool;
    if (get_cur_task()->pde_addr == NULL)
    {
        ASSERT((uint32_t)ptr >= KERNEL_HEAP_BASE);
        mem_pool = &kernel_pool;
        pf = PF_KERNEL;
    }
    else
    {
        mem_pool = &user_pool;
        pf = PF_USER;
    }
    lock_acquire(&mem_pool->lock);
    struct mem_block *__block = (struct mem_block*)ptr;
    struct arena *__arena = block_2_arena(__block);
    if(__arena->desc == NULL)   /* 说明大于1024字节,释放所有页框 */
    {
        page_free(pf, ptr, __arena->cnt);
    }
    else
    {
        /* 回收内存块 */
        list_add_prev(&__arena->desc->free_list, &__block->free_block);

        /* 如果整个arena都空闲，回收页框 */
        if(++(__arena->cnt) == __arena->desc->blocks_each_arena)
        {
            /* 将所有节点从空闲链表删除 */
            uint32_t block_idx = 0;
            for (block_idx = 0; block_idx < __arena->cnt; block_idx++)
            {
                struct mem_block *p_block = arena_2_block(__arena, block_idx);
                // ASSERT(list_find_item(&__arena->desc->free_list, &p_block->free_block));
                list_del_safe(&p_block->free_block);
            }
            page_free(pf, ptr, 1);
        }
    }
    lock_release(&mem_pool->lock);
}

/**
 * mem_init --内存管理初始化
 * 初始化内存池，初始化页表
*/
void mem_init()
{
    put_str("memory initialization start\n");
    /*初始化内存池*/
    mem_pool_init((uint64_t)TOTAL_MEM);
    /*初始化页表*/
    uint32_t kernel_pages = total_kernel_pages((uint64_t)TOTAL_MEM);
    page_table_init(kernel_pages);
    /* 初始化内存块描述符数组 */
    mem_block_desc_init(kernel_block_desc_array);
    put_str("    kernel PDE and PTE reload completion\n");
    put_str("memory initialization completion\n");

    /*测试部分*/
    // put_str("\nbitmap: 0x");
    // put_hex((uint32_t)*(kernel_vm_pool.vm_bitmap.bitmap));
    // put_char('\n');

    // put_hex((uint32_t)page_alloc(1));

    // put_str("\nbitmap: 0x");
    // put_hex((uint32_t)*(kernel_vm_pool.vm_bitmap.bitmap));
    // put_char('\n');

    // put_hex((uint32_t)page_alloc(4));

    // put_str("\nbitmap: 0x");
    // put_hex((uint32_t)*(kernel_vm_pool.vm_bitmap.bitmap));
    // put_char('\n');
}


void print_bitmap()
{
    put_str("\nbitmap: 0x");
    put_hex((uint64_t)*(kernel_vm_pool.vm_bitmap.bitmap));
    put_char('\n');
}