/**
 * 函数get_cur_task()写死了PCB是4KB
*/
#include "../include/os/thread.h"
#include "../include/os/debug.h"
#include "../include/os/memory.h"
#include "../include/os/string.h"
#include "../include/os/interrupt.h"
#include "../include/asm/print.h"


#define PCB_SIZE (PAGE_SIZE * 1)


/*一些全局变量*/
struct task_struct *main_thread;        /*主线程PCB*/
struct general_queue thread_ready_queue;  /*就绪队列*/
struct kernel_list thread_total_list;   /*全部进程链表*/


/*汇编进程调度函数*/
extern void switch_to(struct task_struct *current, struct task_struct *next);


/**
 * get_cur_task --获得当前PCB
*/
struct task_struct* get_cur_task()
{
    uint32_t esp;
    asm("mov %%esp, %0"
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
 * thread_start --初始化一个进程并执行
 * @name: thread name
 * @prio: thread priority
 * @function: the function of process execution
 * @func_argc: function parameter
 */
struct task_struct* thread_start(char* name, int prio, thread_func function, void* func_argc)
{
    struct task_struct *thread = (struct task_struct*)get_kernel_pages(PCB_SIZE / PAGE_SIZE);

    pcb_init(thread, name, prio);
    thread_creat(thread, function, func_argc);

    put_str("function:thread_start\n");
    put_str(thread->name);
    PRINT_ADDR(" PCB", (uint32_t)thread);

    BREAK_POINT(1);
        /*加入就绪队列*/
    // ASSERT(!list_find_item(thread_ready_queue.front, &thread->thread_dispatch_queue));
    queue_in(&thread_ready_queue, &thread->thread_dispatch_queue);
    /*加入进程链表*/
    // ASSERT(!list_find_item(&thread_total_list, &thread->thread_list));

    list_add(&thread_total_list, &thread->thread_list);
    // asm volatile("movl %0, %%esp;\
    //             pop %%ebp;\
    //             pop %%ebx;\
    //             pop %%edi;\
    //             pop %%esi;\
    //             ret"
    //              :
    //              : "g"(thread->self_kernel_stack)
    //              : "memory");
    BREAK_POINT(5);
    current_task = thread;
    return thread;
}

/**
 * init_main_thread --初始化主进程，将main进程
 * 更新为PCB
*/
static void init_main_thread()
{
    /*main进程已经在loader.s中把esp移动到0x9f000
    *PCB地址是0x9e000
    */
    main_thread = get_cur_task();
    pcb_init(main_thread, "main", 31);
    /*将main主进程加入到进程链表中*/
    ASSERT(!list_find_item(&thread_total_list, &main_thread->thread_list));
    list_add(&thread_total_list, &main_thread->thread_list);
}

/**
 * schedule --任务调度器
 * 从就绪队列中获取一个进程，交给switch_to函数进行上下文切换，
 * 并根据当前进程被换下原因，插入到就绪队列尾，或其他处理
*/
void schedule()
{
    local_irq_disable();
    /*schedule被时钟中断服务程序调用，此时CPU应该已经自动关中断*/
    ASSERT(get_intr_status() == INTR_OFF);
    struct task_struct *cur_task = current_task;

    put_str("function:schedule\n");
    put_str(cur_task->name);
    PRINT_ADDR(" PCB", (uint32_t)cur_task);
    PRINT_ADDR("queue", (uint32_t)&cur_task->thread_dispatch_queue);
    PRINT_ADDR("point to PCB", list_entry(0xc1000030, struct task_struct, thread_dispatch_queue));


    /*因为时间片到被换下*/
    if(cur_task->status == TASK_RUNNING)
    {
        ASSERT(!list_find_item(thread_ready_queue.front, &cur_task->thread_dispatch_queue));
        /*加入到就绪队列队尾*/
        BREAK_POINT(9);
        queue_in(&thread_ready_queue, &cur_task->thread_dispatch_queue);
        BREAK_POINT(10);
        /*重置状态*/
        cur_task->ticks = cur_task->priority;
        cur_task->status = TASK_READY;
    }
    else{
        //TODO 因为其他原因被换下，不需要加入就绪队列
    }
    /*从就绪队列中获取一个任务*/
    BREAK_POINT(8);
    struct kernel_list *_next = queue_out(&thread_ready_queue);
    ASSERT(_next != NULL);
    struct task_struct *next_task = list_entry(_next, struct task_struct, thread_dispatch_queue);
    next_task->status = TASK_RUNNING;
    /*执行汇编语言switch_to完成进程切换*/
    switch_to(cur_task, next_task);
}

void thread_init()
{
    put_str("thread initialization start\n");
    list_init(&thread_total_list);
    queue_init(&thread_ready_queue);
    current_task = 0xc009e000;
    init_main_thread();
    put_str("thread initialization completion\n");
}