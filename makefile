BUILD_DIR = ./build
ENTRY_POINT = 0xc0015000
LIB = -I include/asm -I include/os
C_FLAGS = -Wall -m32 $(LIB) -c -fno-stack-protector -fno-builtin
LD_FLAGS = -m elf_i386 -Ttext $(ENTRY_POINT) -e main
OBJS = $(BUILD_DIR)/main.o $(BUILD_DIR)/print.o $(BUILD_DIR)/init.o \
		$(BUILD_DIR)/interrupt.o $(BUILD_DIR)/interrupt_s.o \
		$(BUILD_DIR)/debug.o $(BUILD_DIR)/string.o $(BUILD_DIR)/bitmap.o \
		$(BUILD_DIR)/keyboard.o

##################	NASM编译	################
$(BUILD_DIR)/mbr.bin : boot/mbr.S
	nasm -I boot/include/ boot/mbr.S -o $@
$(BUILD_DIR)/loader.bin : boot/loader.S
	nasm -I boot/include/ boot/loader.S -o $@
$(BUILD_DIR)/print.o : lib/kernel/print.S
	nasm -f elf lib/kernel/print.S -o $@
$(BUILD_DIR)/interrupt_s.o : kernel/interrupt_s.S
	nasm -f elf kernel/interrupt_s.S -o $@


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

################	链接所有目标文件	################
$(BUILD_DIR)/kernel.bin : $(OBJS)
	ld $(LD_FLAGS) $^ -o $@

###################	tools	#################
.PYONY : hd mk_dir clean bochs loader mbr all rebuild rebuildall

mk_dir:
	if [ ! -d $(BUILD_DIR) ];then mkdir $(BUILD_DIR);fi

hd :
	dd if=$(BUILD_DIR)/kernel.bin \
	   of=/usr/local/bochs-2.6.9/hd64.img \
	   bs=512 count=200 seek=9 conv=notrunc

loader :
	dd if=$(BUILD_DIR)/loader.bin \
		of=/usr/local/bochs-2.6.9/hd64.img \
		bs=512 count=4 seek=2 conv=notrunc

mbr :
	dd if=$(BUILD_DIR)/mbr.bin \
		of=/usr/local/bochs-2.6.9/hd64.img \
		bs=512 count=1 seek=0 conv=notrunc

clean: 
	cd $(BUILD_DIR) && rm -f ./*

build : $(BUILD_DIR)/kernel.bin

bochs:
	cd /usr/local/bochs-2.6.9 && bochs -f bochsrc
all: mk_dir build hd bochs
rebuild: clean all
rebuildall: clean $(BUILD_DIR)/mbr.bin $(BUILD_DIR)/loader.bin build mbr loader hd bochs