/*ISSUES
*(1) 打印整数函数put_int	--print.S
*(2) local_irq_save(),local_irq_restore()待重写		--interrupt.h
*/

#include "print.h"
#include "init.h"
#include "interrupt.h"
#include "debug.h"

int main(void) {
	put_str("total memory: 0x");
	uint32_t total_mem = (*(uint32_t *)(0xb00));
	put_hex(total_mem);
	put_char('\n');
	init_os();
	local_irq_enable();
	while(1);
	return 0;
}
