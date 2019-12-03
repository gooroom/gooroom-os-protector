#
# Makefile for OS protector daemon.
#
CONFIG_MODULE_SIG=n

obj-m := shadow_box_helper.o 
shadow_box_helper-objs := helper.o asm.o 

KVERSION = $(shell uname -r)
# Disable function reorder
ccflags-y := -fno-toplevel-reorder

%.o: %.asm
	nasm -f elf64 -o $@ $^

all: 
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) modules 

clean:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) clean

