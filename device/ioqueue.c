#include "../include/os/ioqueue.h"
#include "../include/os/sync.h"
#include "../include/os/interrupt.h"
#include "../include/os/global.h"
#include "../include/os/debug.h"


/**
 * ioqueue_init --初始化io队列
 * @ioq: 队列
*/
void ioqueue_init(struct ioqueue* ioq)
{
    lock_init(&ioq->lock);
    ioq->producer = NULL;
    ioq->consumer = NULL;
    ioq->front = 0;
    ioq->tail = 0;
}

/**
 * ioq_next_pos --返回pos在缓冲区中的下一个位置
 * @pos: 当前位置
*/
static inline uint32_t ioq_next_pos(uint32_t pos)
{
    return (pos + 1) % QUEUE_BUFF_SIZE;
}

/**
 * ioq_full --判断缓冲区是否满
*/
bool ioq_full(struct ioqueue* ioq)
{
    return ioq_next_pos(ioq->front) == ioq->tail;
}

/**
 * ioq_empty --判断缓冲区是否空
*/
bool ioq_empty(struct ioqueue* ioq)
{
    return ioq->front == ioq->tail;
}

/**
 * 使当前生产者或者消费者阻塞
 * @waiter: 指向PCB的二级指针
*/
static void ioq_wait(struct task_struct** waiter)
{
    ASSERT(waiter != NULL && *waiter == NULL);
    *waiter = get_cur_task();
    thread_block(TASK_BLOCKED);
}

/**
 * ioq_wakeup --唤醒waiter
 * @waiter: 指向PCB的二级指针
*/
static void ioq_wakeup(struct task_struct** waiter)
{
    ASSERT(*waiter != NULL);
    thread_unblock(*waiter);
    *waiter = NULL;
}

/**
 * ioq_getchar --从缓冲区中获取一个字符
*/
char ioq_getchar(struct ioqueue* ioq)
{
    ASSERT(get_intr_status() == INTR_OFF);
    /*如果缓冲区为空，登记消费者，并阻塞当前进程*/
    while(ioq_empty(ioq))
    {
        lock_acquire(&ioq->lock);
        ioq_wait(&ioq->consumer);
        lock_release(&ioq->lock);
    }

    char ch = ioq->buff[ioq->tail];
    ioq->tail = ioq_next_pos(ioq->tail);
    /*如果生产者不为空， 则唤醒生产者*/
    if(ioq->producer != NULL)
    {
        ioq_wakeup(&ioq->producer);
    }
    return ch;
}

/**
 * ioq_putchar --向缓冲区中写入一个字符
*/
void ioq_putchar(struct ioqueue* ioq, char ch)
{
    ASSERT(get_intr_status() == INTR_OFF);
    /*如果缓冲区满，登记并阻塞当前进程*/
    while(ioq_full(ioq))
    {
        lock_acquire(&ioq->lock);
        ioq_wait(&ioq->producer);
        lock_release(&ioq->lock);
    }
    /*向缓冲区中写入字符*/
    ioq->buff[ioq->front] = ch;
    ioq->front = ioq_next_pos(ioq->front);
    /*唤醒消费者*/
    if(ioq->consumer != NULL)
    {
        ioq_wakeup(&ioq->consumer);
    }
}
