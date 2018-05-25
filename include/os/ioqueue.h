#ifndef __DEVICE_IOQUEUE_H
#define __DEVICE_IOQUEUE_H
#include "sync.h"
#include "stdint.h"

#define QUEUE_BUFF_SIZE 64

struct ioqueue{
    struct lock lock;
    struct task_struct* producer;
    struct task_struct* consumer;
    char buff[QUEUE_BUFF_SIZE];
    uint32_t front;
    uint32_t tail;
};

void ioqueue_init(struct ioqueue *ioq);
bool ioq_empty(struct ioqueue *ioq);
bool ioq_full(struct ioqueue *ioq);
char ioq_getchar(struct ioqueue *ioq);
void ioq_putchar(struct ioqueue *ioq, char ch);

#endif