# intel-ofs-2023.2-ubuntu-hitek
Installation steps for Intel OFS for Hitek C220 card on Ubuntu 22.04.

> NOTE: the scripts provided in `install/`:
> * may require sudo access;
> * may not run automously due to underlying assumptions, hence, you should keep an eye on the single commands;
> * perform the builds in this directory, where you need r/w access; if you want to change this: `export WORK_DIR=<your dir>

## References:
* OFS github https://github.com/OFS/ofs-agx7-pcie-attach/releases/tag/ofs-2023.2-1
* Hitek SFTP
    * Device support for NC220/C220 card.
> NOTE: target device density for the demo cluster is `014`

## Installation steps

### Hitek release
Extract Hitek FIM release in `hitek_release` folder.

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

### Simulator
>*Requires license*.
Install QuestaSim or VCS. 

### Bringup Hitek C220 PAC
Update PAC firmware. **TBD**

Update BMC FW and RTL:
``` console 
sudo fpgasupdate TBD.rsu
```

Update FIM:
``` console 
sudo fpgasupdate TBD.bin <PCI ADDRESS>
```

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
``` console 
$ source install/initial_afu_setup.sh  
```

<a name="hello_afu"></a>
TBD: from `example-afus`

### `fpgabist`
Platform benchmark with `fpgabist`:
> requires custom afu `.gbs` to be built, i.e., [Hello AFU](#hello_afu)

#### Native Loop-Back
TBD

#### DMA
TBD

