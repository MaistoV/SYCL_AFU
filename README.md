# intel-ofs-2023.2-ubuntu-hitek
Installation steps for Intel OFS for Hitek C220 card on Ubuntu 22.04.

> NOTE: the scripts provided in `install/`:
> * may require sudo access;
> * may not run automously due to underlying assumptions, hence, you should keep an eye on the single commands;
> * perform the builds in this directory, where you need r/w access; if you want to change this: `export INSTALL_BUILD_DIR=<your dir>`

## References
* OFS github https://github.com/OFS/ofs-agx7-pcie-attach/releases/tag/ofs-2023.2-1
* Hitek SFTP
    * Device support for NC220/C220 card.
> NOTE: For the demo cluster, target device is C220 (no network) with device density is `014`

## Bringup Hitek C220 PAC
Update PAC firmware. **TBD**

Update BMC FW and RTL:
``` console 
sudo fpgasupdate TBD.rsu
```

Update FIM:
Set the card PCIe address `PAC_PCIE_SBD` according to your bus. 
> You can find it with `$ lspci | grep bcce | head -n1 | awk '{print $1}'`
``` console 
make fim_update PAC_PCIE_SBD=<ssss:bb:dd.f>
```

## Guides and documentation
1. [Installation steps](install/README.md)
2. [Host Excerciser Modules](HEM/README.md)
3. [AFU flow](afu_flow/README.md)
4. [FIM OFSS flow](fim_flow/README.md)