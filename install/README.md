# Installation steps

## Hitek release
Extract Hitek FIM release in `hitek_release` folder.
> Set `HTS_RELEASE=<your dir>` to the parent directory of your `ofs-agx7-pcie-attach` clone.

## BIOS
* Enable IO-MMU
* Enable VT-d
(NOT WORKING) You can check if they are enabled in the system log:
``` console 
$ sudo dmesg | grep -e DMAR -e IOMMU
```

## Build  and install Linux-DFL and OPAE-SDK
### Build 
Build Linux DFL kernel 6.1.41-dfl.
Run:
``` console 
$ source install/ubuntu/ubuntu_linux_dfl_build.sh # This also installs the packages
or
$ source install/rh8/rh8_linux_dfl_build.sh  
```
Build opae-sdk v2.8.0-1
``` console 
$ source install/ubuntu/ubuntu_opae_build.sh # This also installs the packages
or
$ source install/rh8/rh8_opae_build.sh 
```

### Install 
Installation has been separated from build for RHEL 8.9 for convenience, since it needs to be repeted for each node of the demo cluster. On the other hand, Ubuntu is here used only for developement.
``` console 
$ source install/rh8_9/rh8_9_install.sh  
```

## Quartus Prime Pro 23.2
>*Requires license*.
Install Quartus Prime Pro with **Agilex** support. Including patches (0.02, 0.11, 0.19).
``` console 
$ source install/ubuntu/ubuntu_quartus.sh
```
> Set `QUARTUS_HOME=<your dir>`.

## Simulator
>*Requires license*.
Install QuestaSim or VCS. 
> For QuestaSim, set `MTI_HOME=<questasim home>`

## OneAPI
### Base Toolkit
Install OneAPI Base Toolkit:
``` console 
$ source install/install_oneapi.sh ## WIP
```

### ASP
ASP requires Base Toolkit

### IP Authoring flow
Requires a BSP (WIP)
 
