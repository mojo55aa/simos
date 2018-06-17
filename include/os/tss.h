#ifndef __USERPROG_TSS_H
#define __USERPROG_TSS_H
#include "thread.h"

void set_tss_esp0(struct task_struct *ptask);
void tss_init(void);

#endif