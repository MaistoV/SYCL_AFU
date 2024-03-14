# AFU flow

## Building FIM PR-tree
> TODO: add FIM configuration parameters

Run the following script for cloning the necessary external repos.

``` console 
$ source install/initial_afu_setup.sh  
```

Building the FIM and PR-tree requires multiple hours.
``` console 
$ make fim_build_pr # Multiple hours build 
```

## Add your AFU
Export `AFU_NAME` and create a directory in `afu_flow/afus/` with the following structure:
```  
afu_flow/afus/${AFU_NAME}
    ├── hw/rtl
    |   ├── ${AFU_NAME}.json # Indicate your interface type ofs_plat_afu or afu_main
    |   ├── sources.txt # use only relative paths here
    |   ├── [ofs_plat_afu.sv] # Top-level for full-PIM flow (name is mandatory)
    |   ├── [afu_main.sv] # Top-level for non-PIM flow (name is mandatory)
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

## Simulate the AFU

Simulate, in one terminal setup the ASE enviornment:
``` console 
$ make ase_setup
```
> NOTE: Only a single AFU at PF0.VF0 is detected during ASE simulation.

> NOTE: To launch the simulation again, without rebuilding all sources, just `make ase_launch`.

In another, launch software against the simulation:
``` console 
$ make test_ase
```

## Build GBS
``` console 
$ make gbs # Around 40 minutes build
```

Testing GBS requires to align the hardware and software environment, some make targets are provided:
``` console 
$ make fim_update # Flash the FIM
$ make make pac_powercycle_user1 # Powercycle the PAC
$ make opae.io_bind # Bind VFs
$ make test_gbs # Configure GBS and software against FPGA hardware
```

## SYCL AFUs
Using an AFU name including the substring `"sycl"`, an AFU can be defined in [OneAPI flow](../oneapi/) and exported in AFU flow with:
``` console 
$ make oneapi_ip
``` 
The AFU flow will take care of integrating the necessary sources for simulation and synthesis.

The SYCL IP integration needs to be performed by hand, see the example  [SYCL AFU](afus/sycl_afu).