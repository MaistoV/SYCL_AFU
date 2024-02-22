# OneAPI flow

# Requirements
## Software
OneAPI software sits on top of Intel OFS stack, therefore it needs:
* Linux DFL 
* OPAE-SDK
* Quartus
    * patch 0.2, same as for FIM builds
    * patch [0.2iofs](https://github.com/OFS/ofs-agx7-pcie-attach/blob/release/ofs-2023.2/license/quartus-0.0-0.02iofs-linux.run)

# ASP (Accelerator Support Package)
OneAPI ASP provides one more adapting layer from FIM/PIM to the user.
``` console 
$ source $ONEAPI_ROOT/setvars.sh
$ cd $OFS_ASP_ROOT
$ ./scripts/build-bsp.sh
$ export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$OFS_ASP_ROOT/linux64/lib
```

``` console 
$ aocl diagnose acl0
```

## OneAPI Sample Applications 
Refere to [UG](https://ofs.github.io/ofs-2023.2/hw/common/user_guides/oneapi_asp/ug_oneapi_asp/#26-compile-and-run-oneapi-sample-applications).

``` console 
cd path-to-sample-location
mkdir build
cd build

cmake -DFPGA_DEVICE=full-path-to-oneapi-asp/platform-name:board_variant ..
make report # Optimization report  
make fpga # Multiple hours build, should it not just be the GBS?
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


