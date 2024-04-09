# OneAPI flow

## Software Requirements
OneAPI software sits on top of Intel OFS stack, therefore it needs:
* Linux DFL 
* OPAE-SDK
* Quartus
    * patch 0.2, same as for FIM builds
    * patch [0.2iofs](https://github.com/OFS/ofs-agx7-pcie-attach/blob/release/ofs-2023.2/license/quartus-0.0-0.02iofs-linux.run)

Make sure you installed Linux DFL and OPAE-SDK as in [here](../install/README.md).

# ASP (Accelerator Support Package)
OneAPI ASP provides one more adapting layer from FIM/PIM to the user.

## Install
Do everything with a single script:
``` console 
$ source install/install_oneapi.sh
```

Or apply commands one by one:

* Hitek's release is missing a required library `libpkg_editor.a`. Retrieve it from git:
``` console 
$ mkdir -p $ROOT_DIR/install/downloads/
$ cd $ROOT_DIR/install/downloads/
$ git clone https://github.com/OFS/oneapi-asp.git
$ cd oneapi-asp
$ git checkout tags/ofs-2023.2-1
$ cp $ROOT_DIR/install/downloads/oneapi-asp/common/source/host/lib/libpkg_editor.a $AFS_ASP_ROOT/$ common/source/host/lib/libpkg_editor.a
```

* Build BSP for target PR tree at `$OPAE_PLATFORM_ROOT`:
``` console 
$ export OPAE_PLATFORM_ROOT=<PR tree location>
$ source $ONEAPI_ROOT/setvars.sh
$ cd $OFS_ASP_ROOT
$ ./scripts/build-bsp.sh
$ aocl install ${OFS_ASP_ROOT} # Install BSP for PR tree
```
* If you wish to change the PR-tree you also need to uninstall, rebuild and re-install the BSP. To uninstall:
``` console 
$ aocl uninstall ${OFS_ASP_ROOT}
```

* Export the newly build library path in your envirment:
``` console 
$ export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$OFS_ASP_ROOT/linux64/lib
```

## Build AOCX
Build blue bitstream:
> NOTE: only flat designs are supported with ASP
``` console 
$ make oneapi_asp_build_aocx OFS_ASP_BOARD_VARIANT=<e.g. ofs_nc220>
```
    
Program PAC:
``` console 
$ # Powercyle the PAC with the FIM related to the target PR tree
$ make aocl_aocx_initalize
```

Run diagnostic:
``` console 
$ aocl diagnose acl0
```

## OneAPI SYCL Sample Kernels
Refere to [ASP UG Section](https://ofs.github.io/ofs-2023.2/hw/common/user_guides/oneapi_asp/ug_oneapi_asp/#26-compile-and-run-oneapi-sample-applications).

Build example:

``` console 
$ # cd path-to-sample-location, e.g.:
$ git update submodules
$ cd oneapi/oneAPI-samples/DirectProgramming/C++SYCL_FPGA/ReferenceDesigns/board_test/
$ mkdir build
$ cd build
$ cmake .. \
    -DFPGA_DEVICE=$OFS_ASP_FPGA_DEVICE      \
    -DUSER_HARDWARE_FLAGS="-Xsno-env-check" \
    -DSUPPORTS_USM=1
$ make fpga_emu             # Build emulation    (<test_name>.<fpga_emu>)
$ make fpga_sim             # Build co-simulaton (<test_name>.<fpga_sim>) (15m build)
$ make fpga                 # Build for device   (<test_name>.<fpga>) (1h build)
$ ./<test_name>.<target>    # Run emulation/co-simulation/on device (CL_CONTEXT_MPSIM_DEVICE_INTELFPGA=1 for fpga_sim)
$ make report               # Optimization report  
$ browse <test_name>.report.prj/reports/report.html # Open report
```
In case timing is not met, you can pass `USER_HARDWARE_FLAGS=-Xsseed=seed_value` in the cmake command above and recompile hardware image.

Utility make targets are provided in the top level Makefile:
``` console 
$ make oneapi_asp_<target>
$ make oneapi_asp_report_open # Uses firefox, not fpga_report, for convenience
``` 

# IP Authoring Flow, SYCL AFUs
Following the [install guide](https://www.intel.com/content/www/us/en/docs/programmable/749869/22-4/installing-the-ip-authoring-development.html).

From OneAPI base toolkit, just:
* Intel® Distribution for GDB
* Intel® oneAPI DPC++ Library
* Intel® oneAPI Threading Building Blocks
* Intel® oneAPI DPC++/C++ Compiler
* Intel® VTune™ Profiler

In [afu_flow/afus](afu_flow/afus), create an AFU tree with the same name as your SYCL IP, in [oneapi](oneapi). Note that the name of the IP must include the string `sycl` for the flow to work properly.

Build the SYCL IP and export it in the AFU project:
``` console 
$ make oneapi_ip
``` 

Utility make targets are provided in the top level Makefile:
``` console 
$ make oneapi_ip_fpga_emu # Builds emulation binary $(AFU_NAME)_fpga_emu
$ make oneapi_ip_emu      # Runs emualtion bianty
$ make oneapi_ip_report   # Build IP sources and generate HLD report
$ make clean_oneapi_ip_report # Clean only report and exported project
$ make clean_oneapi_ip    # Clean all IP-related artifacts
$ make oneapi_ip_report_open # View HLD report (uses firefox, not fpga_report, for convenience)
``` 

