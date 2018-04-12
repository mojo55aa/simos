/*ISSUES
*(1) 打印整数函数put_int	--print.S
*(2) local_irq_save(),local_irq_restore()待重写		--interrupt.h
*/

#include "print.h"
#include "init.h"
#include "interrupt.h"
#include "debug.h"
int main(void) {
	put_str("i am kernel\n");
	init_os();
	local_irq_enable();
	while(1);
	return 0;
}
