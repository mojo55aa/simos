#include "process.h"
#include "thread.h"
#include "global.h"
#include "memory.h"
#include "debug.h"
#include "tss.h"
#include "stdint.h"


/*从interrupt.S导入intr_exit函数*/
extern void intr_exit(void);

/**
 * start_process --构建用户进程初始化上下文信息
*/
void start_process(void* filename_)
{
    void *function = filename_;
    struct task_struct* cur_task = get_cur_task();
    cur_task->self_kernel_stack += sizeof(struct task_struct);
    struct intr_stack *proc_stack = (struct intr_stack *)cur_task->self_kernel_stack;
    proc_stack->edi = 0;
    proc_stack->esi = 0;
    proc_stack->ebp = 0;
    proc_stack->esp_old = 0;

    proc_stack->ebx = proc_stack->edx =
        proc_stack->ecx = proc_stack->eax = 0;
    proc_stack->gs = 0;
    proc_stack->ds = proc_stack->es = proc_stack->fs = SELECTOR_U_DATA;
    proc_stack->eip = function;
    proc_stack->cs = SELECTOR_U_CODE;
    proc_stack->eflags = (EFLAGS_IOPL_0 | EFLAGS_MBS | EFLAGS_IF_1);
    proc_stack->ss = SELECTOR_U_STACK;
    //TODO esp待设置
    asm volatile("movl %0, %%esp; jmp intr_exit"
                 :
                 : "g"(proc_stack)
                 : "memory");
}

/**
 * page_dir_activate --激活页表
 * 为用户进程完成页表的更换
 * @ptask: 要切换的进程
*/
void page_dir_activate(struct task_struct* ptask)
{
    /*默认是内核页表地址*/
    uint32_t phy_pde_addr = PAGE_DIR_TABLE;
    if(ptask->pde_addr != NULL)
    {   /*将页目录表虚拟地址转换成物理地址，页目录表在内核空间，线性映射*/
        phy_pde_addr = __pa((uint32_t)ptask->pde_addr);
    }
    asm volatile("movl %0, %%cr3"
                 :
                 : "r"(phy_pde_addr)
                 : "memory");
}

/**
 * process_activate --激活进程
 * (1) 激活页表
 * (2) 更新tss中的esp0
 * @ptask: 待激活的进程
*/
void process_activate(struct task_struct* ptask)
{
    ASSERT(ptask != NULL);
    /*激活页表*/
    page_dir_activate(ptask);
    /*如果是用户进程，更新TSS中的ESP0*/
    if(ptask->pde_addr)
    {
        set_tss_esp0(ptask);
    }
}