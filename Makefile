SHELL := /bin/bash

include version.mk

# DESTDIR is used to install into a different root directory
DESTDIR?=/
# Specify the kernel directory to use
KERNELDIR?=/lib/modules/$(shell uname -r)/build
# Need the absolute directory do the driver directory to build kernel modules
DRIVERDIR?=$(shell pwd)/driver

GUIDIR?=$(shell pwd)/gui
GUI_BINARY := YeetMouseGui
GUI_INSTALL_NAME := yeetmouse
DESKTOP_FILE := install_files/yeetmouse.desktop
ICON_FILE := media/yeetmouse.png

YEETMOUSECTLDIR=$(shell pwd)/tools/yeetmousectl

# Where kernel drivers are going to be installed
MODULEDIR?=/lib/modules/$(shell uname -r)/kernel/drivers/usb

DKMS_VER?=$(YEETMOUSE_VERSION)

# Detect architecture
ARCH := $(shell uname -m)

.PHONY: driver
.PHONY: GUI
.PHONY: userspace
.PHONY: install_userspace install_gui install_gui_files install_gui_optional remove_gui remove_userspace install_config install_service remove_service install_uninstaller remove_uninstaller

default: driver yeetmousectl

all: driver yeetmousectl
clean: driver_clean

GUI:
	@echo -e "\n::\033[32m Building GUI application\033[0m"
	@echo "========================================"
	$(MAKE) -C $(GUIDIR) M=$(GUIDIR)
	@echo "DONE!"

yeetmousectl:
	@echo -e "\n::\033[32m Building yeetmousectl\033[0m"
	@echo "========================================"
	$(MAKE) -C $(YEETMOUSECTLDIR) M=$(YEETMOUSECTLDIR)
	@echo "DONE!"

userspace: yeetmousectl

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
	install -m 644 -v -D version.mk $(DESTDIR)/usr/src/$(DKMS_NAME)-$(DKMS_VER)/version.mk
	install -m 644 -v -D install_files/dkms/dkms.conf $(DESTDIR)/usr/src/$(DKMS_NAME)-$(DKMS_VER)/dkms.conf
	install -m 755 -v -d $(DESTDIR)/usr/src/$(DKMS_NAME)-$(DKMS_VER)/driver
	install -m 755 -v -d $(DESTDIR)/usr/src/$(DKMS_NAME)-$(DKMS_VER)/driver/FixedMath
	install -m 644 -v -D driver/Makefile $(DESTDIR)/usr/src/$(DKMS_NAME)-$(DKMS_VER)/driver/Makefile
	install -m 644 -v driver/*.c $(DESTDIR)/usr/src/$(DKMS_NAME)-$(DKMS_VER)/driver/
	install -m 644 -v driver/*.h $(DESTDIR)/usr/src/$(DKMS_NAME)-$(DKMS_VER)/driver/
	install -m 644 -v driver/FixedMath/*.h $(DESTDIR)/usr/src/$(DKMS_NAME)-$(DKMS_VER)/driver/FixedMath/
	install -m 644 -v shared_definitions.h $(DESTDIR)/usr/src/$(DKMS_NAME)-$(DKMS_VER)/
	$(if $(shell grep "AccelMode_" "$(DRIVERDIR)/config.h"),,@echo "\033[31mWARNING! Old config version detected, acceleration mode might be wrong!\033[0m")
	@rm -fv $(DESTDIR)/usr/src/$(DKMS_NAME)-$(DKMS_VER)/driver/*.mod.c

install_gui_files:
	@echo -e "\n::\033[34m Installing GUI application\033[0m"
	@echo "====================================================="
	install -m 755 -v -D gui/$(GUI_BINARY) $(DESTDIR)/usr/bin/$(GUI_INSTALL_NAME)
	install -m 755 -v -d $(DESTDIR)/usr/share/applications
	install -m 755 -v -d $(DESTDIR)/usr/share/icons/hicolor/256x256/apps
	install -m 644 -v $(DESKTOP_FILE) $(DESTDIR)/usr/share/applications/yeetmouse.desktop
	install -m 644 -v $(ICON_FILE) $(DESTDIR)/usr/share/icons/hicolor/256x256/apps/yeetmouse.png

install_gui: GUI install_gui_files

install_gui_optional:
	@echo -e "\n::\033[34m Attempting optional GUI build/install\033[0m"
	@echo "====================================================="
	@if $(MAKE) GUI; then \
		$(MAKE) install_gui_files; \
		echo "GUI installed."; \
	else \
		echo ""; \
		echo -e "\033[1m\033[1;31mWARNING: GUI was not installed.\033[0m"; \
		echo "Missing GUI build dependencies are the most likely cause (look at the messages above)."; \
		echo "The driver, yeetmousectl, config and service installation will continue."; \
		echo "Install the GUI dependencies for your distribution and run:"; \
		echo -e "  make install_gui\n"; \
	fi

remove_gui:
	@echo -e "\n::\033[34m Removing GUI application\033[0m"
	@echo "====================================================="
	rm -f $(DESTDIR)/usr/bin/$(GUI_INSTALL_NAME)
	rm -f $(DESTDIR)/usr/share/applications/yeetmouse.desktop
	rm -f $(DESTDIR)/usr/share/icons/hicolor/256x256/apps/yeetmouse.png

install_userspace: yeetmousectl
	@echo -e "\n::\033[34m Installing userspace tools\033[0m"
	@echo "====================================================="
	install -m 755 -v -D tools/yeetmousectl/yeetmousectl $(DESTDIR)/usr/bin/yeetmousectl

remove_userspace:
	@echo -e "\n::\033[34m Removing userspace tools\033[0m"
	@echo "====================================================="
	rm -f $(DESTDIR)/usr/bin/yeetmousectl

install_uninstaller:
	@echo -e "\n::\033[34m Installing uninstaller\033[0m"
	@echo "====================================================="
	install -m 755 -v -D install_files/yeetmouse-uninstall.sh $(DESTDIR)/usr/bin/yeetmouse-uninstall

remove_uninstaller:
	@echo -e "\n::\033[34m Removing uninstaller\033[0m"
	@echo "====================================================="
	rm -f $(DESTDIR)/usr/bin/yeetmouse-uninstall

install_config:
	@echo -e "\n::\033[34m Installing default configuration\033[0m"
	@echo "====================================================="
	install -m 755 -v -d $(DESTDIR)/etc
	if [ ! -f $(DESTDIR)/etc/yeetmouse.conf ]; then \
		install -m 644 -v install_files/yeetmouse.conf.sample $(DESTDIR)/etc/yeetmouse.conf; \
	else \
		echo "Keeping existing $(DESTDIR)/etc/yeetmouse.conf"; \
	fi

install_service:
	@echo -e "\n::\033[34m Installing systemd service\033[0m"
	@echo "====================================================="
	install -m 755 -v -d $(DESTDIR)/usr/lib/systemd/system
	install -m 644 -v install_files/systemd/yeetmouse.service $(DESTDIR)/usr/lib/systemd/system/yeetmouse.service

remove_service:
	@echo -e "\n::\033[34m Removing systemd service\033[0m"
	@echo "====================================================="
	rm -f $(DESTDIR)/usr/lib/systemd/system/yeetmouse.service

remove_dkms:
	@echo -e "\n::\033[34m Removing DKMS files\033[0m"
	@echo "====================================================="
	@rm -rf $(DESTDIR)/usr/src/$(DKMS_NAME)-$(DKMS_VER)

install_i_know_what_i_am_doing: all driver_install install_userspace install_config install_service install_uninstaller install_gui
install: manual_install_msg ;

pkgarch:
	@echo -e "\n::\033[34m Building installable arch package\033[0m"
	@echo "====================================================="
	@./scripts/build_arch.sh
	@mv ./pkg/build/yeetmouse*.zst .

manual_install_msg:
	@echo "Please do not install the driver using this method. Use a distribution package as it tracks the files installed and can remove them afterwards. If you are 100% sure, you want to do this, find the correct target in the Makefile."
	@echo "Exiting."

uninstall: driver_uninstall remove_userspace remove_service remove_gui remove_uninstaller