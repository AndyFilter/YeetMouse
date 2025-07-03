obj-m += yeetmouse.o
yeetmouse-objs := accel.o driver.o accel_modes.o

# Detect architecture
ARCH := $(shell uname -m)

# Architecture-specific flags
ifeq ($(ARCH), ppc64le)
    EXTRA_CFLAGS += -D__ppc64le__
    # ccflags-y is the modern kernel build system way to add compiler flags
    ccflags-y += -D__ppc64le__
    $(info Building for PowerPC 64-bit Little Endian)
endif

#ccflags-y += -mhard-float

all:
	cp -n config.sample.h config.h
