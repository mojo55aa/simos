#include "../include/os/sync.h"
#include "../include/os/list.h"
#include "../include/os/global.h"
#include "../include/os/interrupt.h"
#include "../include/os/debug.h"


/**
 * sema_init --初始化信号量
 * @psema: 信号量
 * @values: 信号量初始值
*/
void sema_init(struct semaphore* psema, uint8_t values)
{
    psema->value = values;
    queue_init(&psema->waiter_queue);
}

/**
 * lock_init --初始化锁
 * @plock: 待初始化的锁
*/
void lock_init(struct lock* plock)
{
    plock->holder = NULL;
    plock->holder_repeat_cnt = 0;
    /*二元信号量实现锁，锁初值1*/
    sema_init(&plock->semaphore, 1);
}

/**
 * sema_dowm --信号量down操作(P操作,原子操作)
 * 如果没有可用信号量，则把申请的进程阻塞，加入到信号量等待队列，
 * 如果有可用信号量，信号量值减1
 * @psema: 信号量
*/
void sema_down(struct semaphore* pseam)
{
    /*信号量操作必须是原子操作*/
    enum intr_status old_status = local_irq_disable();
    /*因为进程被唤醒后，需要竞争获得锁，这里用while多次判断条件是否满足*/
    while(pseam->value == 0)    /*如果信号量0，表示被别人持有，需要等待*/
    {
        /*进程不应该在锁的等待队列中*/
        ASSERT(!list_find_item(pseam->waiter_queue.front, &get_cur_task()->thread_dispatch_queue));
        /*如果在等待队列中找到当前进程，则出错*/
        if(list_find_item(pseam->waiter_queue.front, &get_cur_task()->thread_dispatch_queue))
        {
            ASSERT_FAILED("sema down: current thread has been in waiter queue");
        }
        /*将进程加入到信号量的等待队列中*/
        queue_in(&pseam->waiter_queue, &get_cur_task()->thread_dispatch_queue);
        /*阻塞进程*/
        thread_block(TASK_BLOCKED);
        /*进程醒来之后还要进行条件判断，有可能优先级更高的进程又获得了锁*/
    }
    /*values为1或者被唤醒，执行下面的代码， 也就是获得锁*/
    pseam->value--;
    ASSERT(pseam->value == 0);
    /*恢复中断状态*/
    set_intr_status(old_status);
}

/**
 * seam_up --信号量up操作(V操作,原子操作)
 * 如果信号量等待队列不为空，从队列中取出一个进程唤醒，信号量加1
 * @psema: 信号量
*/
void sema_up(struct semaphore* psema)
{
    enum intr_status old_status = local_irq_disable();
    ASSERT(psema->value == 0);
    /*从信号量等待队列中唤醒一个进程*/
    if(!queue_empty(&psema->waiter_queue))
    {
        struct task_struct *blocked_task = list_entry(queue_out(&psema->waiter_queue),\
                                                                 struct task_struct, \
                                                                thread_dispatch_queue);
        thread_unblock(blocked_task);
    }
    /*信号量加1*/
    psema->value++;
    ASSERT(psema->value == 1);
    set_intr_status(old_status);
}

/**
 * lock_qcquire --获得锁
 * 当前进程如果获得了锁，则更新锁信息，如果重复申请，则holder_repeat_cnt++
 * @plock: 锁对象
*/
void lock_acquire(struct lock* plock)
{
    /*锁的持有者不是当前进程*/
    if(plock->holder != get_cur_task())
    {
        sema_down(&plock->semaphore);   /*信号量P操作*/
        plock->holder = get_cur_task();
        ASSERT(plock->holder_repeat_cnt == 0);
        plock->holder_repeat_cnt = 1;
    }
    /*否则说明在重复申请锁*/
    else
    {
        plock->holder_repeat_cnt++;
    }
}

/**
 * lock_release --释放锁
 * 释放锁时更新锁信息和信号量
 * @plock: 锁对象
*/
void lock_release(struct lock* plock)
{
    /*只有自己才能主动释放锁*/
    ASSERT(plock->holder == get_cur_task());
    /*如果有重复申请的情况，则只进行减操作*/
    if(plock->holder_repeat_cnt > 1)
    {
        plock->holder_repeat_cnt--;
        return;
    }
    ASSERT(plock->holder_repeat_cnt == 1);
    plock->holder = NULL;
    plock->holder_repeat_cnt = 0;
    sema_up(&plock->semaphore);     /*信号量V操作*/
}