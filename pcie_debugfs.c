// SPDX-License-Identifier: GPL-2.0
/*
 * pcie_debugfs.c - debugfs interface for PCIe inspector
 *
 * Exposes the following nodes under /sys/kernel/debug/pcie_diag/:
 *
 *   select_dev   rw  write "DDDD:BB:DD.F" to pick the target device
 *   ltssm        r   LTSSM state from DWC PORT_DEBUG0
 *   retrain      w   trigger link retrain
 *   aer_status   r   AER correctable / uncorrectable status
 *   oatu         r   outbound iATU region dump
 *   bar_dump     r   BAR resource map
 *   cap          r   PCIe capability list + link speed/width
 *   iommu        r   IOMMU domain, group, stream IDs, DMA masks
 */

#include <linux/module.h>
#include <linux/pci.h>
#include <linux/iommu.h>
#include <linux/seq_file.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include "pcie_diag.h"

/* ── select_dev ─────────────────────────────────────────────────────────── */

static ssize_t select_dev_write(struct file *file, const char __user *ubuf,
				size_t count, loff_t *pos)
{
	char buf[32];
	unsigned int domain, bus, dev, fn;
	struct pci_dev *pdev;

	if (count >= sizeof(buf))
		return -EINVAL;
	if (copy_from_user(buf, ubuf, count))
		return -EFAULT;
	buf[count] = '\0';

	if (sscanf(buf, "%x:%x:%x.%x", &domain, &bus, &dev, &fn) != 4) {
		pr_err("pcie_diag: invalid format — use DDDD:BB:DD.F\n");
		return -EINVAL;
	}

	pdev = pci_get_domain_bus_and_slot(domain, bus, PCI_DEVFN(dev, fn));
	if (!pdev) {
		pr_err("pcie_diag: device %04x:%02x:%02x.%x not found\n",
		       domain, bus, dev, fn);
		return -ENODEV;
	}

	spin_lock(&dbgfs.lock);
	if (dbgfs.selected_dev)
		pci_dev_put(dbgfs.selected_dev);
	dbgfs.selected_dev = pdev;
	spin_unlock(&dbgfs.lock);

	pr_info("pcie_diag: selected %04x:%02x:%02x.%x\n",
		domain, bus, dev, fn);
	return count;
}

static int select_dev_show(struct seq_file *s, void *data)
{
	spin_lock(&dbgfs.lock);
	if (dbgfs.selected_dev)
		seq_printf(s, "%s\n", pci_name(dbgfs.selected_dev));
	else
		seq_puts(s, "No device selected\n");
	spin_unlock(&dbgfs.lock);
	return 0;
}

static int select_dev_open(struct inode *inode, struct file *file)
{
	return single_open(file, select_dev_show, inode->i_private);
}

static const struct file_operations select_dev_fops = {
	.open    = select_dev_open,
	.read    = seq_read,
	.write   = select_dev_write,
	.llseek  = seq_lseek,
	.release = single_release,
};

/* ── ltssm ──────────────────────────────────────────────────────────────── */

static int ltssm_show(struct seq_file *s, void *data)
{
	struct pci_dev  *pdev = get_selected_dev();
	void __iomem    *dbi;
	u32              ltssm;

	if (!pdev) {
		seq_puts(s, "No device selected\n");
		return 0;
	}

	dbi = pcie_get_region(pdev, "dbi");
	if (!dbi) {
		seq_puts(s, "Failed to map DBI region\n");
		pci_dev_put(pdev);
		return 0;
	}

	ltssm = FIELD_GET(DWC_LTSSM_STATE_MASK, readl(dbi + DWC_PCIE_PORT_DEBUG0));
	seq_printf(s, "LTSSM: %s (%#x)\n",
		   ltssm < dw_pcie_ltssm_str_size ? dw_pcie_ltssm_str[ltssm] : "UNKNOWN",
		   ltssm);

	devm_iounmap(&pdev->dev, dbi);
	pci_dev_put(pdev);
	return 0;
}
DEFINE_SHOW_ATTRIBUTE(ltssm);

/* ── retrain ─────────────────────────────────────────────────────────────── */

static ssize_t retrain_write(struct file *file, const char __user *ubuf,
			     size_t count, loff_t *pos)
{
	struct pci_dev *pdev = get_selected_dev();

	if (!pdev) {
		pr_err("pcie_diag: no device selected\n");
		return -ENODEV;
	}

	pcie_retrain_link(pdev);
	pci_dev_put(pdev);
	return count;
}

static const struct file_operations retrain_fops = {
	.open  = simple_open,
	.write = retrain_write,
};

/* ── aer_status ──────────────────────────────────────────────────────────── */

static int aer_status_show(struct seq_file *s, void *data)
{
	struct pci_dev *pdev = get_selected_dev();
	u32 aer_cap, cor_status, uncor_status, root_status;

	if (!pdev) {
		seq_puts(s, "No device selected\n");
		return 0;
	}

	aer_cap = pci_find_ext_capability(pdev, PCI_EXT_CAP_ID_ERR);
	if (!aer_cap) {
		seq_puts(s, "AER capability not found\n");
		pci_dev_put(pdev);
		return 0;
	}

	pci_read_config_dword(pdev, aer_cap + PCI_ERR_COR_STATUS,   &cor_status);
	pci_read_config_dword(pdev, aer_cap + PCI_ERR_UNCOR_STATUS, &uncor_status);
	pci_read_config_dword(pdev, aer_cap + PCI_ERR_ROOT_STATUS,  &root_status);

	seq_printf(s, "Device       : %s\n", pci_name(pdev));
	seq_printf(s, "COR   status : %#010x\n", cor_status);
	seq_printf(s, "UNCOR status : %#010x\n", uncor_status);
	seq_printf(s, "ROOT  status : %#010x\n", root_status);

	seq_puts(s, "\nCorrectable errors:\n");
	if (cor_status & PCI_ERR_COR_RCVR)     seq_puts(s, "  RxErr\n");
	if (cor_status & PCI_ERR_COR_BAD_TLP)  seq_puts(s, "  BadTLP\n");
	if (cor_status & PCI_ERR_COR_BAD_DLLP) seq_puts(s, "  BadDLLP\n");
	if (cor_status & PCI_ERR_COR_REP_ROLL) seq_puts(s, "  Rollover\n");
	if (cor_status & PCI_ERR_COR_REP_TIMER)seq_puts(s, "  Timeout\n");

	seq_puts(s, "\nUncorrectable errors:\n");
	if (uncor_status & PCI_ERR_UNC_DLP)        seq_puts(s, "  DataLinkError\n");
	if (uncor_status & PCI_ERR_UNC_COMP_ABORT) seq_puts(s, "  CompleterAbort\n");
	if (uncor_status & PCI_ERR_UNC_UNSUP)      seq_puts(s, "  UnsupportedRequest\n");
	if (uncor_status & PCI_ERR_UNC_POISON_TLP) seq_puts(s, "  PoisonedTLP\n");

	seq_puts(s, "\nRoot port:\n");
	if (root_status & PCI_ERR_ROOT_COR_RCV)      seq_puts(s, "  CorrectableReceived\n");
	if (root_status & PCI_ERR_ROOT_NONFATAL_RCV)  seq_puts(s, "  NonFatalReceived\n");
	if (root_status & PCI_ERR_ROOT_FATAL_RCV)     seq_puts(s, "  FatalReceived\n");

	pci_dev_put(pdev);
	return 0;
}
DEFINE_SHOW_ATTRIBUTE(aer_status);

/* ── oatu ────────────────────────────────────────────────────────────────── */

static int oatu_dump_show(struct seq_file *s, void *data)
{
	struct pci_dev *pdev = get_selected_dev();
	void __iomem   *dbi, *atu;
	u32             viewport;

	if (!pdev) {
		seq_puts(s, "No device selected\n");
		return 0;
	}

	dbi = pcie_get_region(pdev, "dbi");
	if (!dbi) {
		seq_puts(s, "Failed to map DBI region\n");
		pci_dev_put(pdev);
		return 0;
	}

	viewport = readl(dbi + DWC_PCIE_ATU_VIEWPORT);
	seq_printf(s, "ATU access: %s\n",
		   (viewport == 0xFFFFFFFF) ? "Unroll" : "Viewport");
	devm_iounmap(&pdev->dev, dbi);

	if (viewport != 0xFFFFFFFF) {
		seq_puts(s, "Viewport-based ATU dump not implemented\n");
		pci_dev_put(pdev);
		return 0;
	}

	atu = pcie_get_region(pdev, "atu_dma");
	if (!atu) {
		seq_puts(s, "Failed to map ATU region\n");
		pci_dev_put(pdev);
		return 0;
	}

	pcie_dump_outbound_atu(s, atu);
	devm_iounmap(&pdev->dev, atu);

	pci_dev_put(pdev);
	return 0;
}
DEFINE_SHOW_ATTRIBUTE(oatu_dump);

/* ── bar_dump ────────────────────────────────────────────────────────────── */

static int bar_dump_show(struct seq_file *s, void *data)
{
	struct pci_dev *pdev = get_selected_dev();
	int bar;

	if (!pdev) {
		seq_puts(s, "No device selected\n");
		return 0;
	}

	for (bar = 0; bar < PCI_STD_NUM_BARS; bar++) {
		resource_size_t start = pci_resource_start(pdev, bar);
		resource_size_t len   = pci_resource_len(pdev, bar);
		unsigned long   flags = pci_resource_flags(pdev, bar);

		if (!start)
			continue;

		seq_printf(s, "BAR%d: start=%pa  size=%pa  flags=%#lx\n",
			   bar, &start, &len, flags);
	}

	pci_dev_put(pdev);
	return 0;
}
DEFINE_SHOW_ATTRIBUTE(bar_dump);

/* ── cap ─────────────────────────────────────────────────────────────────── */

static int cap_show(struct seq_file *s, void *data)
{
	struct pci_dev *pdev = get_selected_dev();
	struct pci_bus *bus;
	u32 devfn;
	u8  pos;
	enum pci_bus_speed speed;
	enum pcie_link_width width;

	if (!pdev) {
		seq_puts(s, "No device selected\n");
		return 0;
	}

	speed = pcie_get_speed_cap(pdev);
	width = pcie_get_width_cap(pdev);

	seq_printf(s, "Speed: %s\n",
		   (speed >= PCIE_SPEED_2_5GT && speed <= PCIE_SPEED_64_0GT)
			? pcie_speed_strs[speed] : "Unknown");
	seq_printf(s, "Width: x%d\n", width);

	/* Walk standard capability list */
	bus   = pdev->bus;
	devfn = pdev->devfn;
	pos   = PCI_CAPABILITY_LIST;

	pci_bus_read_config_byte(bus, devfn, pos, &pos);
	seq_puts(s, "\nCapability list:\n");
	while (pos > PCI_MAX_LAT) {
		u16 cap_id;

		pci_bus_read_config_word(bus, devfn, pos, &cap_id);
		seq_printf(s, "  [%#04x] cap=%#06x\n", pos, cap_id);
		pci_bus_read_config_byte(bus, devfn, pos + 1, &pos);
	}

	pci_dev_put(pdev);
	return 0;
}
DEFINE_SHOW_ATTRIBUTE(cap);

/* ── iommu ───────────────────────────────────────────────────────────────── */

static int iommu_show(struct seq_file *s, void *data)
{
	struct pci_dev      *pdev = get_selected_dev();
	struct device       *dev;
	struct iommu_domain *domain;
	struct iommu_group  *group;
	struct iommu_fwspec *fwspec;
	u32 i;

	if (!pdev) {
		seq_puts(s, "No device selected\n");
		return 0;
	}

	dev = &pdev->dev;

	seq_printf(s, "DMA mask         : %#llx\n",
		   dev->dma_mask ? *dev->dma_mask : 0ULL);
	seq_printf(s, "Coherent DMA mask: %#llx\n", dev->coherent_dma_mask);

	domain = iommu_get_domain_for_dev(dev);
	if (domain) {
		seq_printf(s, "IOMMU domain type: %d\n", domain->type);
		seq_printf(s, "IOMMU pgsize_bitmap: %#lx\n", domain->pgsize_bitmap);
		seq_printf(s, "IOMMU aperture: %#llx – %#llx  force=%s\n",
			   domain->geometry.aperture_start,
			   domain->geometry.aperture_end,
			   domain->geometry.force_aperture ? "yes" : "no");
	} else {
		seq_puts(s, "No IOMMU domain\n");
	}

	group = iommu_group_get(dev);
	if (group) {
		seq_printf(s, "IOMMU group: %d\n", iommu_group_id(group));
		iommu_group_put(group);
	}

	fwspec = dev_iommu_fwspec_get(dev);
	if (fwspec) {
		for (i = 0; i < fwspec->num_ids; i++)
			seq_printf(s, "Stream ID[%u]: %u\n", i, fwspec->ids[i]);
	}

	pci_dev_put(pdev);
	return 0;
}
DEFINE_SHOW_ATTRIBUTE(iommu);

/* ── init / exit ─────────────────────────────────────────────────────────── */

void pcie_debugfs_init(void)
{
	spin_lock_init(&dbgfs.lock);
	dbgfs.selected_dev = NULL;

	dbgfs.dir = debugfs_create_dir("pcie_diag", NULL);

	debugfs_create_file("select_dev", 0644, dbgfs.dir, NULL, &select_dev_fops);
	debugfs_create_file("ltssm",      0444, dbgfs.dir, NULL, &ltssm_fops);
	debugfs_create_file("retrain",    0200, dbgfs.dir, NULL, &retrain_fops);
	debugfs_create_file("aer_status", 0444, dbgfs.dir, NULL, &aer_status_fops);
	debugfs_create_file("oatu",       0444, dbgfs.dir, NULL, &oatu_dump_fops);
	debugfs_create_file("bar_dump",   0444, dbgfs.dir, NULL, &bar_dump_fops);
	debugfs_create_file("cap",        0444, dbgfs.dir, NULL, &cap_fops);
	debugfs_create_file("iommu",      0444, dbgfs.dir, NULL, &iommu_fops);
}

void pcie_debugfs_exit(void)
{
	debugfs_remove_recursive(dbgfs.dir);

	spin_lock(&dbgfs.lock);
	if (dbgfs.selected_dev) {
		pci_dev_put(dbgfs.selected_dev);
		dbgfs.selected_dev = NULL;
	}
	spin_unlock(&dbgfs.lock);
}
