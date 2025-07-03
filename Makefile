# Makefile structure blatantly "stolen" from OpenRazer 3.0.0 and adapted for leetmouse

# DESTDIR is used to install into a different root directory
DESTDIR?=/
# Specify the kernel directory to use
KERNELDIR?=/lib/modules/$(shell uname -r)/build
# Need the absolute directory do the driver directory to build kernel modules
DRIVERDIR?=$(shell pwd)/driver

GUIDIR?=$(shell pwd)/gui

# Where kernel drivers are going to be installed
MODULEDIR?=/lib/modules/$(shell uname -r)/kernel/drivers/usb

DKMS_NAME?=yeetmouse-driver
DKMS_VER?=0.9.2

# Detect architecture
ARCH := $(shell uname -m)

.PHONY: driver
.PHONY: GUI

default: GUI

all: driver
clean: driver_clean

GUI:
	@echo -e "\n::\033[32m Building GUI application\033[0m"
	@echo "========================================"
	#cd $(GUIDIR) && $(MAKE)
	$(MAKE) -C $(GUIDIR) M=$(GUIDIR)
	@echo "DONE!"

driver:
	@echo -e "\n::\033[32m Compiling yeetmouse kernel module\033[0m"
	@echo "========================================"
ifeq ($(ARCH),ppc64le)
	@echo "PowerPC 64-bit Little Endian detected"
endif
	@cp -n $(DRIVERDIR)/config.sample.h $(DRIVERDIR)/config.h || true
	$(MAKE) -C $(KERNELDIR) M=$(DRIVERDIR) modules


driver_clean:
	@echo -e "\n::\033[32m Cleaning yeetmouse kernel module\033[0m"
	@echo "========================================"
	$(MAKE) -C "$(KERNELDIR)" M="$(DRIVERDIR)" clean

# Install kernel modules and then update module dependencies
driver_install:
	@echo -e "\n::\033[34m Installing yeetmouse kernel module\033[0m"
	@echo "====================================================="
	@mkdir -p $(DESTDIR)/$(MODULEDIR)
	@cp -v $(DRIVERDIR)/yeetmouse.ko $(DESTDIR)/$(MODULEDIR)
	@chown -v root:root $(DESTDIR)/$(MODULEDIR)/yeetmouse.ko
	depmod

# Remove kernel modules
driver_uninstall:
	@echo -e "\n::\033[34m Uninstalling yeetmouse kernel module\033[0m"
	@echo "====================================================="
	@rm -fv $(DESTDIR)/$(MODULEDIR)/yeetmouse.ko

setup_dkms:
	@echo -e "\n::\033[34m Installing DKMS files\033[0m"
	@echo "====================================================="
	install -m 644 -v -D Makefile $(DESTDIR)/usr/src/$(DKMS_NAME)-$(DKMS_VER)/Makefile
	install -m 644 -v -D install_files/dkms/dkms.conf $(DESTDIR)/usr/src/$(DKMS_NAME)-$(DKMS_VER)/dkms.conf
	install -m 755 -v -d driver $(DESTDIR)/usr/src/$(DKMS_NAME)-$(DKMS_VER)/driver
	install -m 755 -v -d driver/FixedMath $(DESTDIR)/usr/src/$(DKMS_NAME)-$(DKMS_VER)/driver/FixedMath
	install -m 644 -v -D driver/Makefile $(DESTDIR)/usr/src/$(DKMS_NAME)-$(DKMS_VER)/driver/Makefile
	install -m 644 -v driver/*.c $(DESTDIR)/usr/src/$(DKMS_NAME)-$(DKMS_VER)/driver/
	install -m 644 -v driver/*.h $(DESTDIR)/usr/src/$(DKMS_NAME)-$(DKMS_VER)/driver/
	install -m 644 -v driver/FixedMath/*.h $(DESTDIR)/usr/src/$(DKMS_NAME)-$(DKMS_VER)/driver/FixedMath/
	install -m 644 -v shared_definitions.h $(DESTDIR)/usr/src/$(DKMS_NAME)-$(DKMS_VER)/
	@rm -fv $(DESTDIR)/usr/src/$(DKMS_NAME)-$(DKMS_VER)/driver/*.mod.c

remove_dkms:
	@echo -e "\n::\033[34m Removing DKMS files\033[0m"
	@echo "====================================================="
	@rm -rf $(DESTDIR)/usr/src/$(DKMS_NAME)-$(DKMS_VER)

install_i_know_what_i_am_doing: all driver_install
install: manual_install_msg ;

package:
	@echo -e "\n::\033[34m Building installable package\033[0m"
	@echo "====================================================="
	@./scripts/build_arch.sh
	@mv ./pkg/build/yeetmouse*.zst .

manual_install_msg:
	@echo "Please do not install the driver using this method. Use a distribution package as it tracks the files installed and can remove them afterwards. If you are 100% sure, you want to do this, find the correct target in the Makefile."
	@echo "Exiting."

uninstall: driver_uninstall
