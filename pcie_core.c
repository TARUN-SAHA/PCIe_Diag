// SPDX-License-Identifier: GPL-2.0
/*
 * pcie_core.c - Core helpers: device selection and region mapping
 */

#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include "pcie_diag.h"

/* Global debugfs context — defined once, referenced everywhere */
struct pcie_diag_dbgfs dbgfs;

/* ── device selection ────────────────────────────────────────────────────── */

struct pci_dev *get_selected_dev(void)
{
	struct pci_dev *pdev;

	spin_lock(&dbgfs.lock);
	pdev = dbgfs.selected_dev;
	if (pdev)
		pci_dev_get(pdev);
	spin_unlock(&dbgfs.lock);

	return pdev;
}

/* ── region mapping ──────────────────────────────────────────────────────── */

void __iomem *pcie_get_region(struct pci_dev *pdev, const char *region)
{
	struct pci_host_bridge  *bridge;
	struct platform_device  *plat_dev;
	struct resource         *res;
	void __iomem            *base = NULL;

	if (!pci_is_root_bus(pdev->bus))
		return NULL;

	bridge = pci_find_host_bridge(pdev->bus);
	if (!bridge) {
		dev_err(&pdev->dev, "pcie_diag: host bridge not found\n");
		return NULL;
	}

	plat_dev = to_platform_device(bridge->dev.parent);
	if (!plat_dev) {
		dev_err(&pdev->dev, "pcie_diag: bridge has no platform device\n");
		return NULL;
	}

	res = platform_get_resource_byname(plat_dev, IORESOURCE_MEM, region);
	if (!res) {
		dev_err(&pdev->dev, "pcie_diag: resource '%s' not found\n", region);
		return NULL;
	}

	base = devm_ioremap(&pdev->dev, res->start, resource_size(res));
	if (!base)
		dev_err(&pdev->dev, "pcie_diag: ioremap failed for '%s'\n", region);

	return base;
}
