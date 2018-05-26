#include "syscall.h"
#include "stdint.h"

/* 无参数的系统调用 */
#define _syscall0(NAME) ({				       \
   long __res;					               \
   asm volatile (					       \
   "int $0x80"						       \
   : "=a" (__res)					       \
   : "a" (NAME)					       \
   : "memory"						       \
   );							       \
   __res;						       \
})

/* 一个参数的系统调用 */
#define _syscall1(NAME, ARG1) ({			       \
   long __res;					               \
   asm volatile (					       \
   "int $0x80"						       \
   : "=a" (__res)					       \
   : "a" (NAME), "b" (ARG1)				       \
   : "memory"						       \
   );							       \
   __res;						       \
})

/* 两个参数的系统调用 */
#define _syscall2(NAME, ARG1, ARG2) ({		       \
   long __res;						       \
   asm volatile (					       \
   "int $0x80"						       \
   : "=a" (__res)					       \
   : "a" (NAME), "b" (ARG1), "c" (ARG2)		       \
   : "memory"						       \
   );							       \
   __res;						       \
})

/* 三个参数的系统调用 */
#define _syscall3(NAME, ARG1, ARG2, ARG3) ({		       \
   long __res;						       \
   asm volatile (					       \
      "int $0x80"					       \
      : "=a" (__res)					       \
      : "a" (NAME), "b" (ARG1), "c" (ARG2), "d" (ARG3)       \
      : "memory"					       \
   );							       \
   __res;						       \
})


/**************************** 系统调用接口 *******************************/

/* 返回当前任务pid */
uint32_t getpid()
{
   return _syscall0(__NR_getpid);
}

/* write系统调用 */
uint32_t write(char* str)
{
    return _syscall1(__NR_write, str);
}
