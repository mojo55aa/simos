#ifndef __THREAD_THREAD_H
#define __THREAD_THREAD_H
#include "stdint.h"
#include "list.h"
#include "memory.h"

typedef void thread_func(void *);

typedef int16_t pid_t;              /*pid类型*/

/*进程状态*/
enum task_status
{
    TASK_RUNNING,   /*执行*/
    TASK_READY,     /*就绪*/
    TASK_BLOCKED,   /*阻塞*/
    TASK_WAITING,   /*等待*/
    TASK_HANGING,   /*挂起*/
    TASK_DIED       /*僵死*/
};

/*中断上下文,发生中断时压入数据*/
struct intr_stack{
    uint32_t vec_no;
    /*pushad顺序EAX,ECX,EDX,EBX,ESP,EBP,ESI,EDI*/
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp_old;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;

    uint32_t err_code;
    void (*eip)(void);
    uint32_t cs;
    uint32_t eflags;
    void *esp;
    uint32_t ss;
};


/*线程栈*/
struct thread_stack{
    uint32_t ebp;
    uint32_t ebx;
    uint32_t edi;
    uint32_t esi;

    void (*eip)(thread_func *func, void *func_argv);

    void(*unused_ret);
    thread_func *function;
    void *func_argc;
};

/*PCB*/
struct task_struct{
    uint32_t *self_kernel_stack;                /*进程内核堆栈*/
    enum task_status status;                    /*进程状态*/
    uint16_t priority;                          /*优先级*/
    pid_t pid;                                  /*任务PID*/
    uint8_t ticks;                              /*时钟滴答数*/
    uint32_t elapsed_ticks;                     /*自任务上CPU开始一共的滴答数*/
    char name[32];                              /*进程名字*/
    struct kernel_list thread_dispatch_queue;    /*进程调度队列*/
    struct kernel_list thread_list;             /*进程链表,包括所有进程*/
    uint32_t *pde_addr;                         /*进程页表虚拟地址*/
    struct virtual_mem_pool userprog_vaddr;     /*进程虚拟地址池*/
    uint32_t stack_boundary; /*栈边界，用于检查栈溢出*/
};


/*开始一个线程*/
struct task_struct *thread_start(char *name, int prio, thread_func function, void *func_argc);
struct task_struct *get_cur_task();
void thread_init();
void schedule();
void thread_block(enum task_status set_status);
void thread_unblock(struct task_struct *wake_task);

#endif