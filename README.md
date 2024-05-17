# intel-ofs-2023.2-ubuntu-hitek
Installation steps for Intel OFS for Hitek C220 card on Ubuntu 22.04.

> NOTE: the scripts provided in `install/`:
> * may require sudo access;
> * may not run automously due to underlying assumptions, hence, you should keep an eye on the single commands;
> * perform the builds in this directory, where you need r/w access; if you want to change this: `export INSTALL_BUILD_DIR=<your dir>`

## Software and OFS version Summary
These are the target versions of the project's submodules:
* OFS 2023.2-1
* Hadoop 3.4.0 (latest release to date)
* JMS 2.0
* ActiveMQ 5.16.6
* Java 1.8
* OPAE-SDK 2.8.0-1
* Kernel Linux DFL 6.1.41-dfl
* RHEL 8.9
* ASP oneapi-asp ofs-2023.2-1
* intel-fpga-bbb ofs-2023.2-1
* examples-afu ofs-2023.2-1
* ASE opae-sim 2.8.0
* Quartus 23.2.0 Build 94 06/14/2023 Patches 0.02,0.11,0.19,0.02iofs SC Pro Edition
* OneAPI Base Toolkit 2024.0.2
* OneAPI Samples 2024.0.0






## References
* OFS github https://github.com/OFS/ofs-agx7-pcie-attach/releases/tag/ofs-2023.2-1
* Hitek SFTP
    * Device support for NC220/C220 card.
> NOTE: For the demo cluster, target device is C220 (no network) with device density is `014`

## Find your PAC
Set variable `PAC_PCIE_SBD` for the card PCIe address according to your bus. Add the following to your environment (e.g. bash):
``` console
$ export PAC_PCIE_SBD=$(lspci -D -d 8086:bcce | head -n1 | awk '{print $1}' | sed "s/\.0//g")
```

## Bringup Hitek C220 PAC
Update PAC firmware. **TBD**

Update BMC FW and RTL:
``` console 
sudo fpgasupdate TBD.rsu
```

Update FIM:
``` console 
make fim_update PAC_PCIE_SBD=<ssss:bb:dd.f>
```

## Guides and documentation
Each guide relies on sourcing `settings.sh` in this directory.
> NOTE: You should only source this script **once** per session, otherwise the OneAPI environment would brake.

1. [Installation steps](install/README.md)
2. [Host Excerciser Modules](HEM/README.md)
3. [AFU flow](afu_flow/README.md)
4. [FIM OFSS flow](fim_flow/README.md)
5. [OneAPI](oneapi/README.md) 
6. [SYCL RS Erasure ROMs](oneapi/sycl_rs_erasure/src/rs_erasure/roms/README.md)
7. [Measures](measures/latency/README.md)
