BUILD_DIR = ./build
BOCHS_DIR = /home/format/simos
ENTRY_POINT = 0xc0015000
LIB = -I include/asm -I include/os
C_FLAGS = -Wall -m32 $(LIB) -c -fno-stack-protector -fno-builtin
LD_FLAGS = -m elf_i386 -Ttext $(ENTRY_POINT) -e main
OBJS = $(BUILD_DIR)/main.o $(BUILD_DIR)/print.o $(BUILD_DIR)/init.o \
		$(BUILD_DIR)/interrupt.o $(BUILD_DIR)/interrupt_s.o \
		$(BUILD_DIR)/debug.o $(BUILD_DIR)/string.o $(BUILD_DIR)/bitmap.o \
		$(BUILD_DIR)/keyboard.o $(BUILD_DIR)/memory.o $(BUILD_DIR)/thread.o \
		$(BUILD_DIR)/timer.o $(BUILD_DIR)/thread.o $(BUILD_DIR)/switch.o \
		$(BUILD_DIR)/list.o $(BUILD_DIR)/sync.o $(BUILD_DIR)/console.o \
		$(BUILD_DIR)/ioqueue.o $(BUILD_DIR)/tss.o $(BUILD_DIR)/process.o \
		$(BUILD_DIR)/syscall.o $(BUILD_DIR)/syscall_init.o $(BUILD_DIR)/stdio.o \
		$(BUILD_DIR)/kstdio.o $(BUILD_DIR)/ide.o

##################	NASM编译	################
$(BUILD_DIR)/mbr.bin : boot/mbr.S
	nasm -I boot/include/ boot/mbr.S -o $@
$(BUILD_DIR)/loader.bin : boot/loader.S
	nasm -I boot/include/ boot/loader.S -o $@
$(BUILD_DIR)/print.o : lib/kernel/print.S
	nasm -f elf lib/kernel/print.S -o $@
$(BUILD_DIR)/interrupt_s.o : kernel/interrupt_s.S
	nasm -f elf kernel/interrupt_s.S -o $@
$(BUILD_DIR)/switch.o : thread/switch.S
	nasm -f elf $< -o $@


#################	GCC编译	##################
$(BUILD_DIR)/main.o : kernel/main.c
	gcc $(C_FLAGS) $< -o $@
$(BUILD_DIR)/init.o : kernel/init.c  include/os/init.h include/os/interrupt.h \
			include/asm/print.h
	gcc $(C_FLAGS) $< -o $@
$(BUILD_DIR)/interrupt.o : kernel/interrupt.c include/os/interrupt.h include/os/stdint.h \
			include/os/global.h include/asm/io.h include/asm/print.h
	gcc $(C_FLAGS) $< -o $@
$(BUILD_DIR)/debug.o : kernel/debug.c include/os/interrupt.h include/asm/print.h include/os/debug.h
	gcc $(C_FLAGS) $< -o $@
$(BUILD_DIR)/string.o : lib/string.c include/os/string.h include/os/debug.h include/os/global.h
	gcc $(C_FLAGS) $< -o $@
$(BUILD_DIR)/bitmap.o : lib/kernel/bitmap.c include/os/stdint.h include/os/debug.h include/os/bitmap.h \
			include/os/string.h include/os/global.h
	gcc $(C_FLAGS) $< -o $@
$(BUILD_DIR)/keyboard.o : device/keyboard.c include/os/keyboard.h include/os/stdint.h \
			include/os/interrupt.h include/os/debug.h include/asm/print.h include/asm/io.h
	gcc $(C_FLAGS) $< -o $@
$(BUILD_DIR)/memory.o : mm/memory.c include/os/memory.h include/os/bitmap.h include/os/stdint.h \
			include/asm/print.h
	gcc $(C_FLAGS) $< -o $@
$(BUILD_DIR)/list.o : lib/kernel/list.c include/os/global.h include/os/stdint.h \
			include/os/debug.h
	gcc $(C_FLAGS) $< -o $@
$(BUILD_DIR)/timer.o : device/timer.c include/os/timer.h include/os/interrupt.h include/os/thread.h \
			include/os/debug.h include/asm/print.h
	gcc $(C_FLAGS) $< -o $@
$(BUILD_DIR)/thread.o : thread/thread.c include/os/thread.h include/os/debug.h include/os/memory.h \
			include/os/string.h include/os/interrupt.h include/asm/print.h
	gcc $(C_FLAGS) $< -o $@
$(BUILD_DIR)/sync.o : thread/sync.c include/os/list.h include/os/interrupt.h include/os/debug.h \
			include/os/sync.h include/os/global.h
	gcc $(C_FLAGS) $< -o $@
$(BUILD_DIR)/console.o : device/console.c include/os/console.h include/os/stdint.h include/asm/print.h \
			include/os/sync.h
	gcc $(C_FLAGS) $< -o $@
$(BUILD_DIR)/ioqueue.o : device/ioqueue.c include/os/ioqueue.h include/os/sync.h include/os/global.h \
			include/os/interrupt.h include/os/stdint.h
	gcc $(C_FLAGS) $< -o $@
$(BUILD_DIR)/tss.o : userprog/tss.c include/os/tss.h include/os/global.h include/asm/print.h \
			include/os/thread.h include/os/memory.h include/os/string.h include/os/stdint.h
	gcc $(C_FLAGS) $< -o $@
$(BUILD_DIR)/process.o : userprog/process.c include/os/process.h include/os/global.h include/asm/print.h \
			include/os/thread.h include/os/memory.h include/os/debug.h include/os/tss.h include/os/stdint.h
	gcc $(C_FLAGS) $< -o $@
$(BUILD_DIR)/syscall.o : lib/user/syscall.c include/os/syscall.h include/os/stdint.h
	gcc $(C_FLAGS) $< -o $@
$(BUILD_DIR)/syscall_init.o : userprog/syscall_init.c include/os/syscall_init.h include/os/syscall.h include/asm/print.h \
			include/os/thread.h
	gcc $(C_FLAGS) $< -o $@
$(BUILD_DIR)/stdio.o : lib/stdio.c include/os/stdio.h include/os/stdint.h include/os/string.h include/os/global.h
	gcc $(C_FLAGS) $< -o $@
$(BUILD_DIR)/kstdio.o : lib/kernel/kstdio.c include/os/kstdio.h include/os/stdio.h include/os/console.h include/os/global.h
	gcc $(C_FLAGS) $< -o $@
$(BUILD_DIR)/ide.o : device/ide.c include/os/ide.h include/os/stdio.h include/os/stdio.h include/os/global.h \
			include/os/debug.h include/os/kstdio.h
	gcc $(C_FLAGS) $< -o $@


################	链接所有目标文件	################
$(BUILD_DIR)/kernel.bin : $(OBJS)
	ld $(LD_FLAGS) $^ -o $@

###################	tools	#################
.PYONY : hd mk_dir clean bochs loader mbr all rebuild rebuildall

mk_dir:
	if [ ! -d $(BUILD_DIR) ];then mkdir $(BUILD_DIR);fi

hd :
	dd if=$(BUILD_DIR)/kernel.bin \
	   of=$(BOCHS_DIR)/hd64.img \
	   bs=512 count=200 seek=9 conv=notrunc

loader :
	dd if=$(BUILD_DIR)/loader.bin \
		of=$(BOCHS_DIR)/hd64.img \
		bs=512 count=4 seek=2 conv=notrunc

mbr :
	dd if=$(BUILD_DIR)/mbr.bin \
		of=$(BOCHS_DIR)/hd64.img \
		bs=512 count=1 seek=0 conv=notrunc

clean: 
	cd $(BUILD_DIR) && rm -f ./*

build : $(BUILD_DIR)/kernel.bin

bochs:
	cd $(BOCHS_DIR) && bochs -f bochsrc
all: mk_dir build hd bochs
rebuild: clean all
rebuildall: clean $(BUILD_DIR)/mbr.bin $(BUILD_DIR)/loader.bin build mbr loader hd bochs