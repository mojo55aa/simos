#include "tss.h"
#include "global.h"
#include "stdint.h"
#include "print.h"
#include "string.h"
#include "memory.h"

/* TSS结构 */
struct tss
{
    uint32_t prve_tss;
    uint32_t *esp0;
    uint32_t ss0;
    uint32_t *esp1;
    uint32_t ss1;
    uint32_t *esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t (*eip)(void);
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt;
    uint32_t trace;
    uint32_t io_base;
};

static struct tss tss;

/**
 * set_tss_esp0 --设置tss的esp0字段
 * @ptask: 要执行的任务
*/
void set_tss_esp0(struct task_struct *ptask)
{
    tss.esp0 = (uint32_t *)((uint32_t)ptask + PAGE_SIZE);
}

/**
 * creat_gdt_desc --构建gdt描述符并返回
 * @desc_addr: 描述符段基址
 * @desc_limit: 描述符界限
 * @attr_low: 描述符属性低8位
 * @attr_high: 描述符属性高8位
 * return: 返回创建好的描述符
*/
static struct gdt_desc creat_gdt_desc(uint32_t *desc_addr, uint32_t limit,
                                      uint8_t attr_low, uint8_t attr_high)
{
    uint32_t desc_base = (uint32_t)desc_addr;
    struct gdt_desc desc;
    desc.limit_low_16 = limit & 0x0000ffff;
    desc.base_low_16 = desc_base & 0x0000ffff;
    desc.base_mid_8 = ((desc_base & 0x00ff0000) >> 16);
    desc.attr_low_8 = (uint8_t)(attr_low);
    desc.limit_high_attr_high_8 = (((limit & 0x000f0000) >> 16) + (uint8_t)(attr_high));
    desc.base_high_8 = desc_base >> 24;
    return desc;
}

/**
 * 在gdt中创建tss并重新加载gdt
*/
void tss_init()
{
    put_str("tss initialization start\n");
    uint32_t tss_size = sizeof(tss);
    memset(&tss, 0, tss_size);
    tss.ss0 = SELECTOR_K_STACK;
    tss.io_base = tss_size;
    /*loader.S中定义的GDT表在0x900，tss地址0x900 + 0x20*/
    /* 在gdt中添加dpl为0的tss描述符 */
    *((struct gdt_desc *)0xc0000920) = creat_gdt_desc(
        (uint32_t *)&tss,
        tss_size - 1,
        TSS_ATTR_LOW,
        TSS_ATTR_HIGH);

    /* 在gdt中添加dpl为3的数据段和代码段描述符 */
    *((struct gdt_desc *)0xc0000928) = creat_gdt_desc(
        (uint32_t *)0,
        0xfffff,
        GDT_CODE_ATTR_LOW_DPL3,
        GDT_ATTR_HIGH);

    *((struct gdt_desc *)0xc0000930) = creat_gdt_desc(
        (uint32_t *)0,
        0xfffff,
        GDT_DATA_ATTR_LOW_DPL3,
        GDT_ATTR_HIGH);
    
    /* gdt指针 16位段界限 32位段基址*/
    uint64_t gdt_addr = ((8 * 7 - 1) | ((uint64_t)(uint32_t)0xc0000900 << 16));
    /*重新加载gdt*/
    asm volatile("lgdt %0"
                 :
                 : "m"(gdt_addr));
    /*加载tss*/
    asm volatile("ltr %w0"
                 :
                 : "r"(SELECTOR_TSS));
    put_str("tss initialization completion\n");
}