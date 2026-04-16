// SPDX-License-Identifier: GPL-2.0
obj-m += pcie_diag.o

pcie_diag-objs := \
	main.o          \
	pcie_core.o     \
	pcie_strings.o  \
	pcie_link.o     \
	pcie_atu.o      \
	pcie_debugfs.o

# Build against the running kernel by default
KDIR ?= /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
