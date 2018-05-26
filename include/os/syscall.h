#ifndef __LIB_USER_SYSCALL_H
#define __LIB_USER_SYSCALL_H
#include "stdint.h"

/*系统调用号*/
#define __NR_getpid         0
#define __NR_write          1


/*系统调用导出*/
uint32_t getpid(void);
uint32_t write(char *str);

#endif