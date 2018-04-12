;打印核心模块
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;处理流程;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;（1）备份32位通用寄存器--pushad   入栈顺序:EAX,ECX,EDX,EBX,ESP,EBP,ESI,EDI
;（2）获取光标坐标
;（3）从栈中获取待打印字符
;（4）判断字符类型，CR（回车）,LF (换行),BS (退格)特殊处理，其余进入输出流程
;（5）判断是否需要滚屏
;（6）更新光标坐标值
;（7）恢复寄存器，退出
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
TI_GDT		equ 0
RPL0		equ 0
SELECTOR_VIDEO equ (0x0003<<3) + TI_GDT + RPL0

section .data
put_int_buffer dd 0

[bits 32]
section .text
;--------------------------------------------put_char--------------------------------------
;功能：栈中的一个字符写入光标处
;---------------------------------------------------------------------------------------------
global put_char
put_char:
	pushad
	;设置视频段选择子 
	mov ax, SELECTOR_VIDEO
	mov gs, ax
;--------------获取当前光标位置-------------
;---------------获取高8位
	mov	dx, 0x03d4
	mov 	al, 0x0e
	out 	dx, al
	mov 	dx, 0x03d5
	in	al, dx
	mov 	ah, al
;---------------获取低8位
	mov 	dx, 0x03d4
	mov 	al, 0x0f
	out 	dx, al
	mov 	dx, 0x03d5
	in	al, dx
;----------------光标存入bx
	mov bx, ax
	;在栈中获取待打印的字符
	mov ecx, [esp+36]		;pushad压入4 * 8 = 32 字节数据，加上返回地址，36字节
;----------------下面对字符类型进行判断--------------
;CR=0x0d 	LF=0x0a	BS=0x8
	cmp cl, 0x0d
	jz .is_carriage_return
	cmp cl, 0x0a
	jz .is_line_feed
	cmp cl, 0x8
	jz .is_backspace
	cmp cl, 0x0		;ascii是0，不打印任何内容
	jz .putchar_end

	;不是控制字符，进入输出流程
	jmp .putchar_process
;-----------------------------------------------------------------------------------------------
 .is_backspace:
 ;bx减1，再清除字符
 	dec bx
 	shl bx, 1
 	mov byte [gs:bx], 0x20
 	inc bx
 	mov byte[gs:bx], 0x02
 	shr bx, 1
 	jmp .set_cursor
 ;----------------------------------------------------------------------------------------------
 .putchar_process:
 	shl bx, 1
 	mov byte [gs:bx], cl
 	inc bx
 	mov byte[gs:bx], 0x02
 	shr bx, 1
 	inc bx
 	cmp bx, 2000
 	jl .set_cursor

 .is_carriage_return:
 .is_line_feed:
 	xor dx, dx
 	mov ax, bx
 	mov si, 80
 	div si
 	sub bx, dx

 .is_carriage_return_end:
 	add bx, 80
 	cmp bx, 2000
 .is_line_feed_end:
 	jl .set_cursor

 ;滚屏原理，行范围0-24，将1-24行搬运到0-23行，24行清零
 .roll_screen:
 	cld		;方向标志DF清零，代表SI、DI每次+1或者+2
 	mov ecx, 960	;(2000-80)/4=960次
 	mov esi, 0xc00b80a0
 	mov edi, 0xc00b8000
 	rep movsd
 	;最后一行清零
 	mov ebx, 3840
 	mov ecx, 80
 .cls:
 	mov word [gs:ebx], 0x0720
 	add ebx, 2
 	loop .cls
 	mov bx, 1920
 .set_cursor:
 ;设置高8位
 	mov dx, 0x03d4
 	mov al, 0x0e
 	out dx, al
 	mov dx, 0x03d5
 	mov al, bh
 	out dx, al
 ;设置低8位
 	mov dx, 0x03d4
 	mov al, 0x0f
 	out dx, al
 	mov dx, 0x03d5
 	mov al, bl
 	out dx, al
 .putchar_end:
 	popad
 	ret


 ;----------------------------------------put_str--------------------------------------
 ;功能：打印字符串
 ;---------------------------------------------------------------------------------------
 global put_str
 put_str:
 	push ecx
 	push ebx
 	mov ebx, [esp+12]	;栈中得到字符串地址
 	xor ecx, ecx		;清零，准备存储字符串
.get_char:
	mov cl, [ebx]
	cmp cl, 0		;得到一个字符，判断字符串是否结束'\0'
	jz .str_over
	push ecx
	call put_char
	add esp, 4		;清理栈空间
	inc ebx
	jmp .get_char
.str_over:
	pop ebx
	pop ecx
	ret


;--------------------------------------put_int------------------------------------------
;功能：打印32位整数
;--------------------------------------------------------------------------------------
global put_int
put_int:
	push eax
	push ebx
	push ecx
	mov ecx, [esp+16]	;取出4字节整数



global put_hex
put_hex:
   pushad
   mov ebp, esp
   mov eax, [ebp+4*9]		       ; call的返回地址占4字节+pushad的8个4字节
   mov edx, eax
   mov edi, 7                          ; 指定在put_int_buffer中初始的偏移量
   mov ecx, 8			       ; 32位数字中,16进制数字的位数是8个
   mov ebx, put_int_buffer

;将32位数字按照16进制的形式从低位到高位逐个处理,共处理8个16进制数字
.16based_4bits:			       ; 每4位二进制是16进制数字的1位,遍历每一位16进制数字
   and edx, 0x0000000F		       ; 解析16进制数字的每一位。and与操作后,edx只有低4位有效
   cmp edx, 9			       ; 数字0～9和a~f需要分别处理成对应的字符
   jg .is_A2F 
   add edx, '0'			       ; ascii码是8位大小。add求和操作后,edx低8位有效。
   jmp .store
.is_A2F:
   sub edx, 10			       ; A~F 减去10 所得到的差,再加上字符A的ascii码,便是A~F对应的ascii码
   add edx, 'A'

;将每一位数字转换成对应的字符后,按照类似“大端”的顺序存储到缓冲区put_int_buffer
;高位字符放在低地址,低位字符要放在高地址,这样和大端字节序类似,只不过咱们这里是字符序.
.store:
; 此时dl中应该是数字对应的字符的ascii码
   mov [ebx+edi], dl		       
   dec edi
   shr eax, 4
   mov edx, eax 
   loop .16based_4bits

;现在put_int_buffer中已全是字符,打印之前,
;把高位连续的字符去掉,比如把字符000123变成123
.ready_to_print:
   inc edi			       ; 此时edi退减为-1(0xffffffff),加1使其为0
.skip_prefix_0:  
   cmp edi,8			       ; 若已经比较第9个字符了，表示待打印的字符串为全0 
   je .full0 
;找出连续的0字符, edi做为非0的最高位字符的偏移
.go_on_skip:   
   mov cl, [put_int_buffer+edi]
   inc edi
   cmp cl, '0' 
   je .skip_prefix_0		       ; 继续判断下一位字符是否为字符0(不是数字0)
   dec edi			       ;edi在上面的inc操作中指向了下一个字符,若当前字符不为'0',要恢复edi指向当前字符		       
   jmp .put_each_num

.full0:
   mov cl,'0'			       ; 输入的数字为全0时，则只打印0
.put_each_num:
   push ecx			       ; 此时cl中为可打印的字符
   call put_char
   add esp, 4
   inc edi			       ; 使edi指向下一个字符
   mov cl, [put_int_buffer+edi]	       ; 获取下一个字符到cl寄存器
   cmp edi,8
   jl .put_each_num
   popad
   ret
	


 