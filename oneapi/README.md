# OneAPI flow

# Requirements
## Software
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

Clone repo:

``` console 
$ cd $HTS_RELEASE
$ git clone https://github.com/oneapi-src/oneAPI-samples.git
$ cd oneAPI-samples
$ git checkout tags/2024.0.0 # get compiler version using icpx –version
```

Build example:

``` console 
$ # cd path-to-sample-location, e.g.:
$ cd $HTS_RELEASE/oneAPI-samples/DirectProgramming/C++SYCL_FPGA/ReferenceDesigns/board_test/
$ mkdir build
$ cd build
$ cmake .. \
    -DFPGA_DEVICE=$OFS_ASP_FPGA_DEVICE      \
    -DUSER_HARDWARE_FLAGS="-Xsno-env-check" \
    -DSUPPORTS_USM=1
$ make fpga_emu             # Build emulation    (<test_name>.<fpga_emu>)
$ make fpga_sim             # Build co-simulaton (<test_name>.<fpga_sim>)
$ make fpga                 # Build for device   (<test_name>.<fpga>) (1h build)
$ ./<test_name>.<target>    # Run emulation/co-simulation/on device
$ make report               # Optimization report  
$ browse <test_name>.report.prj/reports/report.html # Open report
```
In case timing is not met, you can pass `USER_HARDWARE_FLAGS=-Xsseed=seed_value` in the cmake command above and recompile hardware image.

# IP Authoring Flow
Following the [install guide](https://www.intel.com/content/www/us/en/docs/programmable/749869/22-4/installing-the-ip-authoring-development.html).

From OneAPI base toolkit, just:
* Intel® Distribution for GDB
* Intel® oneAPI DPC++ Library
* Intel® oneAPI Threading Building Blocks
* Intel® oneAPI DPC++/C++ Compiler
* Intel® VTune™ Profiler

