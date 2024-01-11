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
See install/ubuntu_quartus.sh
>*Requires license*.

### Simulator
Install QuestaSim or VCS. 
>*Requires license*.

### Bringup Hitek NC100 PAC
Update PAC firmware. Download BSP for NC100 from Hitek SFTP.  **TBD**

Update BMC FW and RTL:
``` console 
sudo fpgasupdate AC_BMC_RSU_user_retail_3.2.0_unsigned.rsu
```
> *Required BMC firmware version for 2022 release is:*
> 	* `AC_BMC_RSU_user_retail_3.2.0_unsigned.rsu`
>       * BMC RTL 	3.2.0
>       * BMC NIOS FW 	3.2.0

> Among OFS releases' artifacts, only 3.11 and 3.15 are available

Update FIM:
``` console 
sudo fpgasupdate ofs_top_page1_unsigned_user1.bin <PCI ADDRESS>
```
> *Requires access to SFTP for `ofs_top_page1_unsigned_user1.bin`, refere to 2022-beta releases*

> Probably not compatible with on-PAC BMC available version, i.e, 2.0

## Reboot
Once installation is complete, after every reboot (also warm), run:
``` console 
$ source settings/settings_dfl.sh
$ source settings/settings_opae.sh
```
or 
``` console 
$ source settings.sh
```

### Host Excercisor Modules
Platform testing using Host Excercisor Modules (HEMs)
#### Plaftorm benchmark
In the `tests/` directory, the following sub-directories are available:
 * `freq/` assess user input --clock-mhz impact 
 * `lpbk/` evaluate 2 available AFUs with same GUID
 * `mem/` ?
 * `mem_tg/` ?
 * `test_all/` run with `--testall` flag
 * `trput/` measure max platform bandwidth per cache line reads and interleave patterns

Test results are available in `tests/results/` with file names composed as `<test_name>_<hostname>.csv`.

#### Multithreading testing
Functional verification of thread-safety:
1. ✅ Same user, different VFs, **explicit VF**
2. ❌ Same user, different VFs, implicit VF
3. ❌ Same user, same AFU, multiple available VFs
4. ❌ Same user, same VF
5. ✅ (Parallel) Same user, different VFs, **explicit VF**, in loop


### Hello AFU
<a name="hello_afu"></a>
TBD: from `example-afus`
> *Requires quartus license?*
> *Is license included in OFS release from github*

### `fpgabist`
Platform benchmark with `fpgabist`:
> requires custom afu `.gbs` to be built, i.e., [Hello AFU](#hello_afu)

#### Native Loop-Back
TBD
#### DMA
TBD

