[bits 32]
section .text
global switch_to
switch_to:
    ;这里是自动压入的返回地址
    push esi
    push edi
    push ebx
    push ebp
    mov eax, [esp + 20]
    mov[eax], esp
    ;恢复下一个任务寄存器
    mov eax, [esp + 24]
    mov esp, [eax]
    pop ebp
    pop ebx
    pop edi
    pop esi
    ret