TARGET = dmyfs

# Specify here your kernel dir:
KDIR := /root/kernel/linux-4.15.13
PWD := $(shell pwd)

ccflags-y := -std=gnu99 -Wno-declaration-after-statement

obj-m += $(TARGET).o

dmyfs-objs := dummyfs.o super.o inode.o dir.o file.o

default:
	make -C $(KDIR) SUBDIRS=$(shell pwd) modules

clean:
	make -C $(KDIR) SUBDIRS=$(shell pwd) clean
