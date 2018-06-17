#ifndef __USERPROG_PROCESS_H
#define __USERPROG_PROCESS_H
#include "thread.h"


void start_process(void *filename_);
void page_dir_activate(struct task_struct *ptask);
void process_activate(struct task_struct *ptask);

#endif