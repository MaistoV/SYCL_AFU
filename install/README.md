# Installation steps

## Hitek release
Extract Hitek FIM release in `hitek_release` folder.
> Set `HTS_FIM_RELEASE=<your dir>` to the parent directory of your `ofs-agx7-pcie-attach` clone.

## BIOS
* Enable IO-MMU
* Enable VT-d

## linux-dfl
Install Linux DFL kernel 6.1-1.
Run:
``` console 
$ source install/ubuntu_linux_dfl_build.sh  
```

## opae-sdk
Build and install opae-sdk v2.8.0-1
``` console 
$ source install/ubuntu_opae_build.sh  
```

## Quartus Prime Pro 23.2
>*Requires license*.
Install Quartus Prime Pro with **Agilex** support. Including patches (0.02, 0.11, 0.19).
``` console 
$ source install/ubuntu_quartus.sh
```
> Set `QUARTUS_HOME=<your dir>`.

## Simulator
>*Requires license*.
Install QuestaSim or VCS. 
> For QuestaSim, set `MTI_HOME=<questasim home>`
