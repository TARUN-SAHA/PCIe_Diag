/* SPDX-License-Identifier: GPL-2.0 */
/*
 * pcie_diag.h - PCIe debug/inspection driver for DesignWare controllers
 *
 * Provides debugfs interface to inspect PCIe link state, ATU regions,
 * AER status, IOMMU mappings, and BAR resources.
 */

#ifndef _PCIE_INSPECTOR_H
#define _PCIE_INSPECTOR_H

#include <linux/pci.h>
#include <linux/seq_file.h>
#include <linux/debugfs.h>
#include <linux/spinlock.h>

/* ── DesignWare ATU ──────────────────────────────────────────────────────── */

#define DWC_OUTBOUND_ATU_REGIONS		8

#define PCIE_ATU_IB_UNROLL_BASE(i)		(((i) << 9) | BIT(8))
#define PCIE_ATU_OB_UNROLL_BASE(i)		((i) << 9)

#define PCIE_ATU_ENABLE				BIT(31)
#define PCIE_CTRL1_INC_REGION_SIZE		BIT(13)

#define PCIE_ATU_REGION_CTRL1			0x000
#define PCIE_ATU_REGION_CTRL2			0x004
#define PCIE_ATU_LOWER_BASE			0x008
#define PCIE_ATU_UPPER_BASE			0x00C
#define PCIE_ATU_LIMIT				0x010
#define PCIE_ATU_LOWER_TARGET			0x014
#define PCIE_ATU_UPPER_TARGET			0x018
#define PCIE_ATU_UPPER_LIMIT			0x020

/* ── DesignWare Port Debug / LTSSM ──────────────────────────────────────── */

#define DWC_PCIE_PORT_DEBUG0			0x728
#define DWC_LTSSM_STATE_MASK			0x03F
#define DWC_PCIE_ATU_VIEWPORT			0x900

/* ── TLP type field in ATU CTRL1 ────────────────────────────────────────── */

#define TLP_TYPE_MASK				0x01F
#define TLP_TYPE_MEM				0x000
#define TLP_TYPE_IO				0x002
#define TLP_TYPE_CFG				0x004
#define TLP_TYPE_CPL				0x008
#define TLP_TYPE_MSG				0x010

/* ── LTSSM state strings ─────────────────────────────────────────────────── */

extern const char * const dw_pcie_ltssm_str[];
extern const size_t       dw_pcie_ltssm_str_size;

/* ── Speed / TLP string tables ──────────────────────────────────────────── */

extern const char * const pcie_speed_strs[];
extern const char * const tlp_type_strs[];

/* ── debugfs context ────────────────────────────────────────────────────── */

/**
 * struct pcie_diag_dbgfs - global debugfs state
 * @dir:          root debugfs directory
 * @selected_dev: currently selected PCI device (ref-counted)
 * @lock:         protects selected_dev
 */
struct pcie_diag_dbgfs {
	struct dentry  *dir;
	struct pci_dev *selected_dev;
	spinlock_t      lock;
};

extern struct pcie_diag_dbgfs dbgfs;

/* ── helpers ─────────────────────────────────────────────────────────────── */

/**
 * get_selected_dev() - return selected pci_dev with elevated refcount
 *
 * Caller must call pci_dev_put() when done.
 * Returns NULL if no device is selected.
 */
struct pci_dev *get_selected_dev(void);

/**
 * pcie_get_region() - ioremap a named resource from the host bridge
 * @pdev:   endpoint device (must be on root bus)
 * @region: DT resource name (e.g. "dbi", "atu_dma")
 *
 * Returns ioremapped base or NULL on failure.
 * Caller must devm_iounmap() when done.
 */
void __iomem *pcie_get_region(struct pci_dev *pdev, const char *region);

/* ── subsystem init/exit ─────────────────────────────────────────────────── */

void pcie_debugfs_init(void);
void pcie_debugfs_exit(void);

/* ── link control ────────────────────────────────────────────────────────── */

void pcie_retrain_link(struct pci_dev *pdev);

/* ── ATU ─────────────────────────────────────────────────────────────────── */

void pcie_dump_outbound_atu(struct seq_file *s, void __iomem *atu_base);

#endif /* _PCIE_INSPECTOR_H */
