# SPDX-License-Identifier: GPL-2.0+
# Copyright (C) 2019-2021 Maximilian Luz <luzmaximilian@gmail.com>

MODULE_NAME := "surface_aggregator_module"
MODULE_VERSION := "0.1"

KVERSION := "$(shell uname -r)"
KDIR := /lib/modules/$(KVERSION)/build
MDIR := /usr/src/$(MODULE_NAME)-$(MODULE_VERSION)

CHECKPATCH_OPTS := -f -q --no-tree --ignore LONG_LINE
CHECKPATCH := $(KDIR)/scripts/checkpatch.pl $(CHECKPATCH_OPTS)

SSAM_MODPROBE := $(shell pwd)/../scripts/ssam-modprobe


sources-dkms := dkms.conf
sources-dkms += Makefile

sources-c := $(shell find . -type f \( -name "*.c" -and -not -name "*.mod.c" \))
sources-h := $(shell find . -type f -name "*.h")
sources-Kbuild := $(shell find . -type f -name "Kbuild")

sources := $(sources-c) $(sources-h) $(sources-Kbuild) $(sources-dkms)


ifneq (,$(shell grep "struct ssam_device_id" $(KDIR)/include/linux/mod_devicetable.h))
	config += __KERNEL_HAS_SSAM_MODALIAS_SUPPORT__=y
endif


all:
	$(MAKE) -C $(KDIR) M=$(shell pwd)/src $(config) modules

clean:
	$(MAKE) -C $(KDIR) M=$(shell pwd)/src $(config) clean

install:
	$(MAKE) -C $(KDIR) M=$(shell pwd)/src $(config) modules_install

%.check:
	@$(CHECKPATCH) $(basename $@) || true

check:
	@$(CHECKPATCH) $(sources-c) $(sources-h)

dkms-install:
	@for f in $(sources); do		\
		mkdir -p `dirname $(MDIR)/$$f`;	\
		cp -v $$f $(MDIR)/$$f;		\
	done
	dkms add $(MODULE_NAME)/$(MODULE_VERSION)
	dkms build $(MODULE_NAME)/$(MODULE_VERSION)
	dkms install $(MODULE_NAME)/$(MODULE_VERSION)

dkms-uninstall:
	modprobe -r $(MODULE_NAME) || true
	dkms uninstall $(MODULE_NAME)/$(MODULE_VERSION) || true
	dkms remove $(MODULE_NAME)/$(MODULE_VERSION) --all || true
	rm -rf $(MDIR)

insmod:
	@cd src/ && sudo $(SSAM_MODPROBE) insmod

rmmod:
	@cd src/ && sudo $(SSAM_MODPROBE) rmmod

modprobe-load:
	@sudo $(SSAM_MODPROBE)

modprobe-unload:
	@sudo $(SSAM_MODPROBE) -r
