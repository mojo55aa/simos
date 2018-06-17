[bits 32]
%define ERROR_CODE nop
%define ZERO push 0

extern irqaction_t	;导入中断服务程序数组

section .data
;导出中断入口表
global intr_entry_table

intr_entry_table:
%macro VECTOR 2
section .text
intr_%1_entry:
	%2		;压入中断错误码

	;保存上下文
	push ds
	push es
	push fs
	push gs
	pushad

	;如果从片进入中断，从片和主片都要发送EOI
	mov al, 0x20	;EOI信号
	out 0xa0, al
	out 0x20, al

	push %1	;压入中断向量号
	call [irqaction_t + %1*4]	;调用C版本中断处理函数
	jmp intr_exit

section .data
	dd intr_%1_entry
%endmacro

section .text
global intr_exit
intr_exit:
	;恢复上下文，从中断返回
	add esp, 4	;中断号
	popad
	pop gs
	pop fs
	pop es
	pop ds
	add esp, 4	;error_code
	iretd
;0-31异常和非屏蔽中断
VECTOR 0x00,ZERO
VECTOR 0x01,ZERO
VECTOR 0x02,ZERO
VECTOR 0x03,ZERO 
VECTOR 0x04,ZERO
VECTOR 0x05,ZERO
VECTOR 0x06,ZERO
VECTOR 0x07,ZERO 
VECTOR 0x08,ERROR_CODE
VECTOR 0x09,ZERO
VECTOR 0x0a,ERROR_CODE
VECTOR 0x0b,ERROR_CODE 
VECTOR 0x0c,ZERO
VECTOR 0x0d,ERROR_CODE
VECTOR 0x0e,ERROR_CODE
VECTOR 0x0f,ZERO 
VECTOR 0x10,ZERO
VECTOR 0x11,ERROR_CODE
VECTOR 0x12,ZERO
VECTOR 0x13,ZERO 
VECTOR 0x14,ZERO
VECTOR 0x15,ZERO
VECTOR 0x16,ZERO
VECTOR 0x17,ZERO 
VECTOR 0x18,ERROR_CODE
VECTOR 0x19,ZERO
VECTOR 0x1a,ERROR_CODE
VECTOR 0x1b,ERROR_CODE 
VECTOR 0x1c,ZERO
VECTOR 0x1d,ERROR_CODE
VECTOR 0x1e,ERROR_CODE
VECTOR 0x1f,ZERO

;外部可屏蔽中断
VECTOR 0x20,ZERO	;定时器时钟中断
VECTOR 0x21,ZERO	;键盘中断
VECTOR 0x22,ZERO	;主从片级联
VECTOR 0x23,ZERO	;tty2
VECTOR 0x24,ZERO	;tty1
VECTOR 0x25,ZERO	;XT Winchester	并口硬盘(IDE)
VECTOR 0x26,ZERO	;软驱
VECTOR 0x27,ZERO	;打印机
VECTOR 0x28,ZERO	;实时时钟
VECTOR 0x29,ZERO	;重定向到IRQ2
VECTOR 0x2a,ZERO	;保留
VECTOR 0x2b,ZERO	;保留
VECTOR 0x2c,ZERO	;PS/2鼠标
VECTOR 0x2d,ZERO	;FPU浮点单元异常
VECTOR 0x2e,ZERO	;AT Winchester 串口硬盘(ATA)
VECTOR 0x2f,ZERO	;保留

[bits 32]
extern syscall_table
section .text
global syscall_handler
syscall_handler:
	;保存上下文环境
	push 0		;错误码占位
	push ds
	push es
	push fs
	push gs
	pushad		;eax ecx edx ebx esp ebp esi edi
	push 0x80	;压入中断号
	;传入系统调用参数
	push edx	;第3个参数
	push ecx	;第2个参数
	push ebx	;第1个参数
	;调用子功能处理函数，子功能号在eax寄存器中
	call [syscall_table + eax*4]
	add esp, 12
	;将返回值存入内核栈中eax的位置，从中断返回自动pop到eax寄存器
	mov [esp + 8*4], eax	;ABI约定，结果保存在eax中,push 0x80压入4字节,pushad eax最先入栈，压入8个寄存器
	jmp intr_exit