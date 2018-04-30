#include "../include/os/thread.h"
#include "../include/os/debug.h"
#include "../include/os/memory.h"
#include "../include/os/string.h"


#define PCB_SIZE (PAGE_SIZE * 1)


static void kernel_func(thread_func* function, void* func_argc)
{
    function(func_argc);
}

void thread_creat(struct task_struct* pthread, thread_func* function, void* func_argc)
{
    /*预留中断栈*/
    pthread->self_kernel_stack -= sizeof(struct intr_stack);
    /*预留线程栈*/
    pthread->self_kernel_stack -= sizeof(struct thread_stack);
    struct thread_stack *kernel_stack = (struct thread_stack *)pthread->self_kernel_stack;
    kernel_stack->eip = kernel_func;
    kernel_stack->func_argc = func_argc;
    kernel_stack->function = function;
    kernel_stack->ebp = 0;
    kernel_stack->ebx = 0;
    kernel_stack->edi = 0;
    kernel_stack->esi = 0;
}

/*初始化PCB*/
void pcb_init(struct task_struct* pthread, char* name, int prio)
{
    memset(pthread, 0, sizeof(*pthread));
    strcpy(pthread->name, name);
    pthread->priority = prio;
    pthread->status = TASK_RUNNING;
    pthread->self_kernel_stack = (uint32_t *)(uint32_t)(pthread + PCB_SIZE);
    pthread->stack_boundary = 0x20130901;
}

/*创建一个进程*/
struct task_struct* thread_init(char* name, int prio, thread_func function, void* func_argc)
{
    //BREAK_POINT(1);
    struct task_struct *thread = (struct task_struct*)get_kernel_pages(PCB_SIZE / PAGE_SIZE);
    //PRINT_ADDR("", (uint32_t)thread);
    //BREAK_POINT(2);
    pcb_init(thread, name, prio);

    //BREAK_POINT(3);
    thread_creat(thread, function, func_argc);

    //BREAK_POINT(4);
    asm volatile("movl %0, %%esp;\
                pop %%ebp;\
                pop %%ebx;\
                pop %%edi;\
                pop %%esi;\
                ret"\
                 :  \
                 : "g"(thread->self_kernel_stack)\
                 : "memory");
    BREAK_POINT(5);
    return thread;
}
