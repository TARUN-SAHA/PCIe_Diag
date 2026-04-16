// SPDX-License-Identifier: GPL-2.0
/*
 * pcie_atu.c - DesignWare iATU region dump
 */

#include <linux/pci.h>
#include <linux/seq_file.h>
#include <linux/io.h>
#include "pcie_diag.h"

void pcie_dump_outbound_atu(struct seq_file *s, void __iomem *atu_base)
{
	int region;

	for (region = 0; region < DWC_OUTBOUND_ATU_REGIONS; region++) {
		void __iomem *rb = atu_base + PCIE_ATU_OB_UNROLL_BASE(region);
		u32 cr1, ctrl2;
		u8  tlp_type;
		u32 base_lo, base_hi, limit_lo, limit_hi, tgt_lo, tgt_hi;
		u64 base, limit, target;

		ctrl2 = readl(rb + PCIE_ATU_REGION_CTRL2);
		if (!(ctrl2 & PCIE_ATU_ENABLE))
			continue;

		cr1      = readl(rb + PCIE_ATU_REGION_CTRL1);
		tlp_type = cr1 & TLP_TYPE_MASK;

		base_lo  = readl(rb + PCIE_ATU_LOWER_BASE);
		base_hi  = readl(rb + PCIE_ATU_UPPER_BASE);
		limit_lo = readl(rb + PCIE_ATU_LIMIT);
		limit_hi = readl(rb + PCIE_ATU_UPPER_LIMIT);
		tgt_lo   = readl(rb + PCIE_ATU_LOWER_TARGET);
		tgt_hi   = readl(rb + PCIE_ATU_UPPER_TARGET);

		base   = ((u64)base_hi  << 32) | base_lo;
		limit  = (((u64)limit_hi & 0x7) << 32) | limit_lo;
		target = ((u64)tgt_hi   << 32) | tgt_lo;

		seq_printf(s, "Outbound ATU #%d  type=%s\n",
			   region,
			   (tlp_type <= TLP_TYPE_MSG && tlp_type_strs[tlp_type])
				? tlp_type_strs[tlp_type] : "UNKNOWN");
		seq_printf(s, "  CPU base : %#llx\n", base);
		seq_printf(s, "  limit    : %#llx\n", limit);
		seq_printf(s, "  PCI addr : %#llx\n", target);
	}
}
