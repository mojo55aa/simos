#include "../include/os/thread.h"
#include "../include/os/debug.h"
#include "../include/os/memory.h"
#include "../include/os/string.h"
#include "../include/os/interrupt.h"


#define PCB_SIZE (PAGE_SIZE * 1)


/*一些全局变量*/
struct task_struct *main_thread;        /*主线程PCB*/
struct general_queue thread_ready_queue;  /*就绪队列*/
struct kernel_list thread_total_list;   /*全部进程链表*/


/*汇编进程调度函数*/
extern switch_to(struct task_struct *current, struct task_struct *next);


/**
 * get_cur_pcb --获得当前PCB
*/
struct task_struct* get_cur_pcb()
{
    uint32_t esp;
    asm volatile("mov %%esp, %0"
                 : "=g"(esp));
    return (struct task_struct *)(esp & 0xfffff000);
}



/**
 * 
*/
static void kernel_func(thread_func* function, void* func_argc)
{
    local_irq_enable();     /*开中断，进行进程调度*/
    function(func_argc);
}


/**
 * thread_creat --创建进程
 * @pthread: 进程PCB指针
 * @function: 进程执行的函数
 * @func_argc: 进程函数参数
*/
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

/**
 * pcb_init --初始化PCB
 * @pthread: PCB指针
 * @name: 进程名字
 * @prio: 进程优先级
 */
void pcb_init(struct task_struct* pthread, char* name, int prio)
{
    memset(pthread, 0, sizeof(*pthread));
    strcpy(pthread->name, name);
    /*主进程设置为运行态，其他进程设置就绪态*/
    if(pthread == main_thread)
    {
        pthread->status = TASK_RUNNING;
    }
    else
    {
        pthread->status = TASK_READY;
    }

    pthread->ticks = prio;
    pthread->priority = prio;
    pthread->elapsed_ticks = 0;
    pthread->pde_addr = NULL;
    // pthread->status = TASK_RUNNING;
    pthread->self_kernel_stack = (uint32_t *)(uint32_t)(pthread + PCB_SIZE);
    pthread->stack_boundary = 0x20130901;
}

/**
 * thread_init --初始化一个进程并执行
 * @name: thread name
 * @prio: thread priority
 * @function: the function of process execution
 * @func_argc: function parameter
 */
struct task_struct* thread_init(char* name, int prio, thread_func function, void* func_argc)
{
    //BREAK_POINT(1);
    struct task_struct *thread = (struct task_struct*)get_kernel_pages(PCB_SIZE / PAGE_SIZE);
    //PRINT_ADDR("", (uint32_t)thread);
    //BREAK_POINT(2);
    pcb_init(thread, name, prio);

    /*加入就绪队列*/
    ASSERT(!list_find_item(&thread_ready_queue.front, &thread->thread_dispatch_queue));
    list_add(&thread_ready_queue.front, &thread->thread_dispatch_queue);
    /*加入进程链表*/
    ASSERT(!list_find_item(&thread_total_list, &thread->thread_list));
    list_add(&thread_total_list, &thread->thread_list);

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

static void init_main_thread()
{
    /*main进程已经在loader.s中把esp移动到0x9f000
    *PCB地址是0x9e000
    */
    main_thread = get_cur_pcb();
    pcb_init(main_thread, "main", 31);
    /*将main主进程加入到进程链表中*/
    ASSERT(!list_find_item(&thread_total_list, &main_thread->thread_list));
    list_add(&thread_total_list, &main_thread->thread_list);
}