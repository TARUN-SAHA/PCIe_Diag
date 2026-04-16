// SPDX-License-Identifier: GPL-2.0
/*
 * pcie_link.c - PCIe link control (retrain, speed/width change)
 */

#include <linux/pci.h>
#include <linux/delay.h>
#include "pcie_diag.h"

#define RETRAIN_TIMEOUT_MS	1000

void pcie_retrain_link(struct pci_dev *pdev)
{
	u16 lnkctl, lnksta;
	u32 cap;
	int timeout = RETRAIN_TIMEOUT_MS;

	cap = pci_find_capability(pdev, PCI_CAP_ID_EXP);
	if (!cap) {
		dev_err(&pdev->dev, "pcie_diag: PCIe capability not found\n");
		return;
	}

	pci_read_config_word(pdev, cap + PCI_EXP_LNKCTL, &lnkctl);
	lnkctl |= PCI_EXP_LNKCTL_RL;
	pci_write_config_word(pdev, cap + PCI_EXP_LNKCTL, lnkctl);

	/* Poll until Link Training bit clears */
	while (timeout--) {
		pci_read_config_word(pdev, cap + PCI_EXP_LNKSTA, &lnksta);
		dev_dbg(&pdev->dev, "link_status: %#x\n", lnksta);
		if (!(lnksta & PCI_EXP_LNKSTA_LT))
			break;
		msleep(1);
	}

	if (timeout <= 0)
		dev_err(&pdev->dev, "pcie_diag: link retrain timed out\n");
	else
		dev_info(&pdev->dev, "pcie_diag: link retrained successfully\n");
}
