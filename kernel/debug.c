#include "debug.h"
#include "print.h"
#include "interrupt.h"

void debug_out(const char* filename, \
                int line, \
                const char* func, \
                const char* condition)
{
    local_irq_disable();
    put_str("\n\n======SYSTEM HANG UP======\n");
    put_str("filename: ");
    put_str((char*)filename);
    put_str("\n");
    put_str("line: ");
    put_str((char*)line);
    put_str("\n");
    put_str("function: ");
    put_str((char*)func);
    put_str("\n");
    put_str("condition: ");
    put_str((char*)condition);
    put_str("\n");
    while(1);   /*挂起*/
}