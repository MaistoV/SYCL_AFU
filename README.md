# intel-ofs-2022.2-ubuntu-hitek
Installation steps for Intel OFS for Hitek NC100 card on Ubuntu 22.04.

> NOTE: the scripts provided in `install/`:
> * require sudo access;
> * may not run automously due to underlying assumptions, hence, you should keep an eye on the single commands;
> * perform the builds from `/home/intelFPGA`, where you need r/w access; if you want to change this: `export WORK_DIR=<your dir>`

## References:
* OTCshare (private repo, snapshot at `otcshare_dumps/`)
    * Original $USER guide for RHEL is at `otcshare_dumps/intel-ofs-docs-main/n6000/$USER_guides/ofs_getting_started/ug_qs_ofs_n6000.md` .

* OFS github 
    * Oldest public release is 2023.1 and has significantly changed since 2022.1.
* Hitek SFTP
    * Device support for HiPrAcc™ NC100 card, a.k.a. Agilex Low Profile PCIe Card.

## Installation steps
### BIOS
* Enable IO-MMU
* Enable VT-d

### linux-dfl
Install linux kernel 5.15 with dfl config.
Run:
``` console 
$ source install/ubuntu_linux_dfl_build.sh  
```

### opae-sdk
Build and install opae-sdk v2.1.1-1
``` console 
$ source install/ubuntu_opae_build.sh  
```

### Quartus Prime Pro
Install Quartus Prime Pro with **Agilex** support. 
>*Requires license*.

### Simulator
Install QuestaSim or VCS. 
>*Requires license*.

### Bringup Hitek NC100 PAC
Update PAC firmware. Download BSP for NC100 from Hitek SFTP.  **TBD**

Update BMC FW and RTL
> *Required BMC firmware version for 2022 release is:*
> 	* AC_BMC_RSU_user_retail_3.2.0_unsigned.rsu
>    * BMC RTL 	3.2.0
>    * BMC NIOS FW 	3.2.0
``` console 
sudo fpgasupdate AC_BMC_RSU_user_retail_3.2.0_unsigned.rsu
```

Update FIM:
> *Requires access to SFTP for ofs_top_page1_unsigned_user1.bin*
``` console 
sudo fpgasupdate ofs_top_page1_unsigned_user1.bin <PCI ADDRESS>
```

### Host Excercisor Modules
#### Plaftorm benchmark
TODO: script for complete platform benchmarking

#### Multithreading testing
TDB: Using HEMs

## Reboot
After every reboot (also warm), run:
``` console 
$ source settings/settings_dfl.sh
$ source settings/settings_opae.sh
```
