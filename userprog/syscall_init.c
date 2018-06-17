#include "syscall_init.h"
#include "syscall.h"
#include "thread.h"
#include "print.h"
#include "console.h"
#include "string.h"
#include "memory.h"


#define SYSCALL_CNT     32  /*目前系统调用数量*/
typedef void *p_syscall;

p_syscall syscall_table[SYSCALL_CNT];   /*系统调用表*/

/**
 * register_syscall --注册系统调用
 * 在syscall_table表中添加条目
 * @number: 系统调用号
 * @func: 系统调用函数
*/
static inline void register_syscall(uint32_t number, p_syscall func)
{
    syscall_table[number] = func;
}

/*************************** 系统调用具体实现 ********************************/

/**
 * sys-getpid --返回当前任务PID
*/
uint32_t sys_getpid(void)
{
    return get_cur_task()->pid;
}

/**
 * sys_write --打印字符串str
*/
uint32_t sys_write(char* str)
{
    console_str(str);
    return strlen(str);
}


/**
 * syscall_init 系统调用初始化
 * 注册所有的系统调用
*/
void syscall_init(void)
{
    put_str("syscall initialization start\n");
    register_syscall(__NR_getpid, sys_getpid);
    register_syscall(__NR_write, sys_write);
    register_syscall(__NR_malloc, sys_malloc);
    register_syscall(__NR_free, sys_free);
    put_str("syscall initialization complation\n");
}