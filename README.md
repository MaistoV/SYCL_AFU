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
$ make fim_pr 
```

#### Run an AFU
Set `AFU_NAME=<...>`, and add a script `afu_flow/afus/afu_${AFU_NAME}.sh`, which sets the following variables:
1. `AFU_ELF_NAME`   : for host executable
2. `AFU_SOURCE_LIST`: e.g. location of sources.txt
3. `AFU_SW_DIR`     : location of sofware sources and Makefile

Provided examples for `AFU_NAME`:
* host_chan_mmio
* hello_world
* dma

Then, for each terminal session below:
``` console 
$ source ${ROOT_DIR}/afu_flow/settings_afu.sh
```

Simulate, in one terminal:
``` console 
$ make ase_setup
```
> To launch the simulation again, without rebuilding all sources, just `make ase_launch`.

In another:
``` console 
$ make test_ase
```

Build and test GBS:
``` console 
$ make gbs # Multiple hours build
$ make test_gbs
```

