# FIM OFSS flow
Set `OFSS_CONFIG` variable and create directory in `fim_flow/ofss_configs/`
``` 
fim_flow/ofss_configs/ofss_config_${OFSS_CONFIG}
    ├── pcie
    |   └── pcie_host_${OFSS_CONFIG}.ofss
    ├── memory (untested)
    ├── iopll (untested)
    ├── hssi (not used)
    └── htk-nc220-agf014_${OFSS_CONFIG}.ofss
```

Run FIM build with:
``` console
$ export OFSS_CONFIG=<config name>
$ make fim_build_<pr|flat>
``` 

## Number of VFs
To increase the number of VFs exposed in the PR region, you must increase the `num_vfs` under `[pf0]`. This number is then expoed in the FIM PCIe descriptors and through the sysfs at `/sys/devices/<....>/0000:01:00.0/sriov_totalvfs` (given the PAC PCIe address is `0000:01:00`).

Preliminary experiments show that 10 VFs can be routed on the PR-region for the system target frequency. Further analysis si necessary to determine the number of VFs necessary to meet the system's throughput requirement.

### Caveat 
Preliminary experiments show that the 8th VF, is mapped to a PF 0 of a subsequent device number, e.g. `0000:01:01.0`, which will not bind to a vfio-pci driver by the OPAE library. E.g.:

``` console
$ opae.io ls | sort
[0000:01:00.0] (0x8086:0xbcce 0x8086:0x1771) Intel Open FPGA Stack Platform NC220 (Driver: dfl-pci)
[0000:01:00.1] (0x8086:0xbcce 0x8086:0x1771) Intel Open FPGA Stack Platform NC220 (Driver: vfio-pci)
[0000:01:00.2] (0x8086:0xbcce 0x8086:0x1771) Intel Open FPGA Stack Platform NC220 (Driver: vfio-pci)
[0000:01:00.3] (0x8086:0xbcce 0x8086:0x1771) Intel Open FPGA Stack Platform NC220 (Driver: vfio-pci)
[0000:01:00.4] (0x8086:0xbcce 0x8086:0x1771) Intel Open FPGA Stack Platform NC220 (Driver: vfio-pci)
[0000:01:00.5] (0x8086:0xbccf 0x8086:0x1771) Intel Open FPGA Stack Platform NC220 (Driver: vfio-pci)
[0000:01:00.6] (0x8086:0xbccf 0x8086:0x1771) Intel Open FPGA Stack Platform NC220 (Driver: vfio-pci)
[0000:01:00.7] (0x8086:0xbccf 0x8086:0x1771) Intel Open FPGA Stack Platform NC220 (Driver: vfio-pci)
[0000:01:01.0] (0x8086:0xbccf 0x8086:0x1771) Intel Open FPGA Stack Platform NC220 (Driver: dfl-pci) # The issue is here
[0000:01:01.1] (0x8086:0xbccf 0x8086:0x1771) Intel Open FPGA Stack Platform NC220 (Driver: vfio-pci)
[0000:01:01.2] (0x8086:0xbccf 0x8086:0x1771) Intel Open FPGA Stack Platform NC220 (Driver: vfio-pci)
[0000:01:01.3] (0x8086:0xbccf 0x8086:0x1771) Intel Open FPGA Stack Platform NC220 (Driver: vfio-pci)
[0000:01:01.4] (0x8086:0xbccf 0x8086:0x1771) Intel Open FPGA Stack Platform NC220 (Driver: vfio-pci)
[0000:01:01.5] (0x8086:0xbccf 0x8086:0x1771) Intel Open FPGA Stack Platform NC220 (Driver: vfio-pci)
[0000:01:01.6] (0x8086:0xbccf 0x8086:0x1771) Intel Open FPGA Stack Platform NC220 (Driver: vfio-pci)
```

This causes any hardware mapped in hardware on `0000:01:01.0` not to be enumerated by the vfio plugin, and therefore not accessible. E.g., of 10 placed VFs, only 9 would be accessible by OPAE-SDK.

Furthermore, all the switching resources are wasted for this unreachable port.

#### Solution
When using this flow, be aware of not plugging any hardware to the 8th port and tying it off. For an example see [ofs_plat_afu_array.sv](../afu_flow/afus/sycl_afu_common/hw/rtl/ofs_plat_afu_array.sv)