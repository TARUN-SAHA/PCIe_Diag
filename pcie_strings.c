// SPDX-License-Identifier: GPL-2.0
/*
 * pcie_strings.c - String tables for PCIe state/type decoding
 */

#include <linux/pci.h>
#include "pcie_diag.h"

/* Indexed by PORT_DEBUG0[5:0] — must stay in sync with DW LTSSM encoding */
const char * const dw_pcie_ltssm_str[] = {
	[0x00] = "DETECT_QUIET",
	[0x01] = "DETECT_ACT",
	[0x02] = "POLL_ACTIVE",
	[0x03] = "POLL_COMPLIANCE",
	[0x04] = "POLL_CONFIG",
	[0x05] = "PRE_DETECT_QUIET",
	[0x06] = "DETECT_WAIT",
	[0x07] = "CFG_LINKWD_START",
	[0x08] = "CFG_LINKWD_ACCEPT",
	[0x09] = "CFG_LANENUM_WAIT",
	[0x0A] = "CFG_LANENUM_ACCEPT",
	[0x0B] = "CFG_COMPLETE",
	[0x0C] = "CFG_IDLE",
	[0x0D] = "RCVRY_LOCK",
	[0x0E] = "RCVRY_SPEED",
	[0x0F] = "RCVRY_RCVRCFG",
	[0x10] = "RCVRY_IDLE",
	[0x11] = "L0",
	[0x12] = "L0S",
	[0x13] = "L123_SEND_EIDLE",
	[0x14] = "L1_IDLE",
	[0x15] = "L2_IDLE",
	[0x16] = "L2_WAKE",
	[0x17] = "DISABLED_ENTRY",
	[0x18] = "DISABLED_IDLE",
	[0x19] = "DISABLED",
	[0x1A] = "LPBK_ENTRY",
	[0x1B] = "LPBK_ACTIVE",
	[0x1C] = "LPBK_EXIT",
	[0x1D] = "LPBK_EXIT_TIMEOUT",
	[0x1E] = "HOT_RESET_ENTRY",
	[0x1F] = "HOT_RESET",
	[0x20] = "RCVRY_EQ0",
	[0x21] = "RCVRY_EQ1",
	[0x22] = "RCVRY_EQ2",
	[0x23] = "RCVRY_EQ3",
	/* Vendor-specific pseudo substates from get_ltssm() */
	[0x24] = "L1SS_L1_1",
	[0x25] = "L1SS_L1_2",
	[0x26] = "UNKNOWN",
};

const size_t dw_pcie_ltssm_str_size = ARRAY_SIZE(dw_pcie_ltssm_str);

/* Indexed by pcie_link_speed enum (0x14–0x19) */
const char * const pcie_speed_strs[] = {
	[PCIE_SPEED_2_5GT]  = "2.5 GT/s (Gen1)",
	[PCIE_SPEED_5_0GT]  = "5.0 GT/s (Gen2)",
	[PCIE_SPEED_8_0GT]  = "8.0 GT/s (Gen3)",
	[PCIE_SPEED_16_0GT] = "16.0 GT/s (Gen4)",
	[PCIE_SPEED_32_0GT] = "32.0 GT/s (Gen5)",
	[PCIE_SPEED_64_0GT] = "64.0 GT/s (Gen6)",
};

/* Indexed by TLP_TYPE_* defines */
const char * const tlp_type_strs[] = {
	[TLP_TYPE_MEM] = "MEM",
	[TLP_TYPE_IO]  = "IO",
	[TLP_TYPE_CFG] = "CFG",
	[TLP_TYPE_CPL] = "CPL",
	[TLP_TYPE_MSG] = "MSG",
};
