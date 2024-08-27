# Installation steps

## Hitek release
Extract Hitek FIM release in `hitek_release` folder.
> Set `HTS_RELEASE=<your dir>` to the parent directory of your `ofs-agx7-pcie-attach` clone.

## BIOS
* Enable PCIe hot-plug (for RSU)
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

On the building node, after build, send the built packages to the other nodes:
``` console 
$ source install/scp_package.sh  
```
> NOTE: this scripts assumes to send from node rh8-50 to all others (51-59). Modify the script's `addresses` variable as needed.

On each target node, you will find the following tree:

```
~/install/
├── check_install.sh        # Check installation (not automated)
├── package                 # Pre-built packages
│   └── rpm                 # RPMs
│       ├── linux-dfl       # Linux-DFL RPMs
│       │   ├── kernel-6.1.41_dfl_dirty-1.x86_64.rpm
│       │   └── kernel-headers-6.1.41_dfl_dirty-1.x86_64.rpm
│       └── opae-sdk        # OPAE-SDK RPMs
│           ├── opae-2.8.0-1.el8.x86_64.rpm
│           ├── opae-debuginfo-2.8.0-1.el8.x86_64.rpm
│           ├── opae-debugsource-2.8.0-1.el8.x86_64.rpm
│           ├── opae-devel-2.8.0-1.el8.x86_64.rpm
│           ├── opae-devel-debuginfo-2.8.0-1.el8.x86_64.rpm
│           ├── opae-extra-tools-2.8.0-1.el8.x86_64.rpm
│           └── opae-extra-tools-debuginfo-2.8.0-1.el8.x86_64.rpm
├── rh8_9_install.sh        # Install script
└── rh8_9_prerequisites.sh  # Install prerequisites (ran by rh8_9_install.sh)
```

Run the install script:
``` console 
$ source install/rh8_9/rh8_9_install.sh  
```
> NOTE: Requires sudo access and proper dnf subscription.

## Quartus Prime Pro 23.2
>*Requires license*.
Install Quartus Prime Pro with **Agilex** support. Including patches (0.02, 0.11, 0.19).
``` console 
$ source install/ubuntu/ubuntu_quartus.sh
```
> Set `QUARTUS_HOME=<your dir>`.

For RHEL, install also libnsl.

## Simulator
>*Requires license*.
Install QuestaSim or VCS. 
> For QuestaSim, set `MTI_HOME=<questasim home>` in your environment.

## Memory limits configuration
> NOTE: if you are also installing [OneAPI](#OneAPI), this configuration can be skipped.
Sending and receiving data from and to the FPGA usually requires large buffers. Therefore, we must configure the memory limits in this system. Run:
``` console 
$ source install/config_memlock_limits.sh
```

## OneAPI<a id="OneAPI"></a>
### Base Toolkit
Install OneAPI Base Toolkit:
``` console 
$ source install/install_oneapi.sh
```

