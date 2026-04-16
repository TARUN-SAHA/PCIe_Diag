# pcie_diag

A lightweight PCIe debug kernel module for **DesignWare**-based controllers
(tested on NVIDIA Tegra / Jetson Orin). Provides a `debugfs` interface to
inspect PCIe link state, ATU regions, AER status, IOMMU mappings, and BAR
resources — without modifying the platform driver.

---

## File structure

```
pcie_diag/
├── main.c            # module init/exit, device enumeration
├── pcie_diag.h  # shared definitions, macros, extern declarations
├── pcie_core.c       # device selection, region ioremap helpers
├── pcie_strings.c    # LTSSM / speed / TLP type string tables
├── pcie_link.c       # link retrain
├── pcie_atu.c        # outbound iATU region dump
├── pcie_aer.c        # AER inject helpers
├── pcie_debugfs.c    # all debugfs file operations
└── Makefile
```

---

## Build

```bash
# native (running kernel)
make

# cross-compile for Jetson (aarch64)
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- KDIR=/path/to/tegra-kernel
```

---

## Usage

```bash
# load
sudo insmod pcie_diag.ko

# list detected devices in dmesg
dmesg | grep pcie_diag

# select a device by domain:bus:dev.fn
echo "0008:00:00.0" > /sys/kernel/debug/pcie_diag/select_dev

# confirm selection
cat /sys/kernel/debug/pcie_diag/select_dev

# read LTSSM state
cat /sys/kernel/debug/pcie_diag/ltssm

# dump outbound ATU regions
cat /sys/kernel/debug/pcie_diag/oatu

# check AER status
cat /sys/kernel/debug/pcie_diag/aer_status

# dump BAR resources
cat /sys/kernel/debug/pcie_diag/bar_dump

# dump capability list + speed/width
cat /sys/kernel/debug/pcie_diag/cap

# dump IOMMU domain / stream IDs
cat /sys/kernel/debug/pcie_diag/iommu

# trigger link retrain
echo 1 > /sys/kernel/debug/pcie_diag/retrain

# unload
sudo rmmod pcie_diag
```

---

## debugfs nodes

| Node         | Mode | Description                                      |
|--------------|------|--------------------------------------------------|
| `select_dev` | rw   | Write `DDDD:BB:DD.F` to select target device     |
| `ltssm`      | r    | LTSSM state from DWC PORT_DEBUG0                 |
| `retrain`    | w    | Trigger PCIe link retrain                        |
| `aer_status` | r    | AER correctable / uncorrectable / root status    |
| `oatu`       | r    | Outbound iATU region dump (unroll mode)          |
| `bar_dump`   | r    | BAR start / size / flags                         |
| `cap`        | r    | Standard capability list + link speed/width      |
| `iommu`      | r    | IOMMU domain, group ID, stream IDs, DMA masks    |

---

## Requirements

- Kernel ≥ 5.10
- `CONFIG_PCIEAER=y`
- `CONFIG_IOMMU_API=y`
- `CONFIG_DEBUG_FS=y`
- DesignWare PCIe controller with `dbi` and `atu_dma` named resources in DT

---

## License

GPL-2.0
