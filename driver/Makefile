obj-m += leetmouse.o
leetmouse-objs := accel.o usbmouse.o

ccflags-y += -mhard-float -mpreferred-stack-boundary=4

all:
	cp -n config.sample.h config.h
