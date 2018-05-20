#ifndef __THREAD_SYNC_H
#define __THREAD_SYNC_H
#include "list.h"
#include "stdint.h"
#include "thread.h"

/*信号量结构*/
struct semaphore{
    uint8_t value;  /*信号量值*/
    struct general_queue waiter_queue;     /*在此信号量上等待的所有线程*/
};

/*锁结构*/
struct lock{
    struct task_struct* holder;      /*锁的持有者*/
    struct semaphore semaphore;     /*锁包含的额信号量，二元信号量实现锁初值为1*/
    uint32_t holder_repeat_cnt;     /*记录持有者重复申请的次数，避免多次释放锁*/
};

void sema_init(struct semaphore *psema, uint8_t values);
void lock_init(struct lock *plock);
void sema_down(struct semaphore *pseam);
void sema_up(struct semaphore *psema);
void lock_acquire(struct lock *plock);
void lock_release(struct lock *plock);

#endif