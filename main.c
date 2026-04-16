// SPDX-License-Identifier: GPL-2.0
/*
 * pcie_diag.c - Module entry/exit
 *
 * A lightweight PCIe debug driver for DesignWare-based controllers (e.g.
 * NVIDIA Tegra). Provides a debugfs interface to inspect link state, ATU
 * regions, AER status, IOMMU mappings, and BAR resources without requiring
 * controller-specific driver modifications.
 *
 * Usage:
 *   insmod pcie_diag.ko
 *   echo "0008:00:00.0" > /sys/kernel/debug/pcie_diag/select_dev
 *   cat  /sys/kernel/debug/pcie_diag/ltssm
 */

#include <linux/module.h>
#include <linux/pci.h>
#include "pcie_diag.h"

static int __init pcie_diag_init(void)
{
	struct pci_dev *pdev = NULL;

	pr_info("pcie_diag: loaded\n");
	pr_info("pcie_diag: available devices:\n");

	for_each_pci_dev(pdev) {
		pr_info("  %04x:%02x:%02x.%d  vendor=%04x device=%04x class=%#x  root=%s\n",
			pci_domain_nr(pdev->bus),
			pdev->bus->number,
			PCI_SLOT(pdev->devfn),
			PCI_FUNC(pdev->devfn),
			pdev->vendor,
			pdev->device,
			pdev->class,
			pci_is_root_bus(pdev->bus) ? "yes" : "no");
	}

	pcie_debugfs_init();
	return 0;
}

static void __exit pcie_diag_exit(void)
{
	pcie_debugfs_exit();
	pr_info("pcie_diag: unloaded\n");
}

module_init(pcie_diag_init);
module_exit(pcie_diag_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tarun Saha");
MODULE_DESCRIPTION("PCIe inspector/debug driver for DesignWare controllers");
