# intel-ofs-2023.2-ubuntu-hitek
Installation steps for Intel OFS for Hitek C220 card on Ubuntu 22.04.

> NOTE: the scripts provided in `install/`:
> * may require sudo access;
> * may not run automously due to underlying assumptions, hence, you should keep an eye on the single commands;
> * perform the builds in this directory, where you need r/w access; if you want to change this: `export INSTALL_BUILD_DIR=<your dir>`

## References:
* OFS github https://github.com/OFS/ofs-agx7-pcie-attach/releases/tag/ofs-2023.2-1
* Hitek SFTP
    * Device support for NC220/C220 card.
> NOTE: For the demo cluster, target device is C220 (no network) with device density is `014`

## Installation steps

### Hitek release
Extract Hitek FIM release in `hitek_release` folder.
> Set `HTS_FIM_RELEASE=<your dir>` to the parent directory of your `ofs-agx7-pcie-attach` clone.

### BIOS
* Enable IO-MMU
* Enable VT-d

### linux-dfl
Install Linux DFL kernel 6.1-1.
Run:
``` console 
$ source install/ubuntu_linux_dfl_build.sh  
```

### opae-sdk
Build and install opae-sdk v2.8.0-1
``` console 
$ source install/ubuntu_opae_build.sh  
```

### Quartus Prime Pro 23.2
>*Requires license*.
Install Quartus Prime Pro with **Agilex** support. Including patches (0.02, 0.11, 0.19).
``` console 
$ source install/ubuntu_quartus.sh
```
> Set `QUARTUS_HOME=<your dir>`.

### Simulator
>*Requires license*.
Install QuestaSim or VCS. 
> For QuestaSim, set `MTI_HOME=<questasim home>`

### Bringup Hitek C220 PAC
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

### AFU flow
Run the following script for cloning the necessary external repos and building the FIM and PR-tree.
> Building the FIM requires multiple hours.
``` console 
$ source install/initial_afu_setup.sh  
$ make fim_build_pr # Multiple hours build 
```

#### Add your AFU
Create a directory in `afu_flow/afus/` with the following structure:
``` console 
afu_flow/afus/${AFU_NAME}
    ├── hw/rtl
    |   ├── ${AFU_NAME}.json
    |   ├── sources.txt
    |   ├── [ofs_plat_afu.sv] # Top-level for full-PIM flow (name is mandatory)
    |   ├── [afu_main.sv] # Top-level for non-full-PIM flow (name is mandatory)
    |   └── <other rtl soruces>
    └── sw
        ├── Makefile # Template file available in afu_flow/afus/common/sw/
        ├── ${AFU_NAME}.c
        └── <other software sources>
```
> You need either `ofs_plat_afu.sv` or `afu_main.sv`, when choosing between PIM-based flows

Provided examples for `AFU_NAME`:
* my_custom_afu
* my_custom_afu_array
* ...

Simulate, in one terminal setup the ASE enviornment:
``` console 
$ make ase_setup
```
> To launch the simulation again, without rebuilding all sources, just `make ase_launch`.

In another, launch software against the simulation:
``` console 
$ make test_ase
```

Build and test GBS:
``` console 
$ make gbs # Aroud 40 minutes build
$ make test_gbs # Launch software against FPGA hardware
```

