#ifndef __KERNEL_DEBUG_H
#define __KERNEL_DEBUG_H

/*打印错误信息
*文件名
*行号
*函数名
*条件
*/
void debug_out(const char* filename, int line, const char* func, const char* condition);

/*通过定义宏，把编译器参数传递给debug_out函数
*__VA_ARGS__对应于省略号中的参数
*/
#define ASSERT_FAILED(...) debug_out(__FILE__, __LINE__, __func__, __VA_ARGS__)

#ifdef NODEBUG      /*如果没有打开DEBUG开关，则ASSERT什么也不做,(void)0 防止做右值*/
    #define ASSERT ((void)0)
#else
    #define ASSERT(CONDITION) do{\
    /*如果断言条件成立，则什么也不做，否则输出错误信息，系统挂起*/\
        if(CONDITION){}\
        else{\
            ASSERT_FAILED(#CONDITION);\
        }}while(0)
#endif  /*NODEBUG*/

#endif  /*__KERNEL_DEBUG_H*/