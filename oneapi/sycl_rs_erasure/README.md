# SYCL ReedSolomon Kernel
These sources are meant for IP Authoring flow, nevertheless they are compatible with ASP runtime.

It is based on CMake, so most of the variables are locked at com

## Required Environment
Compared with the other SYCL exaples, the CMake flow defines more features:
1. It requires some envvars to be set:
    * RS_SCHEMA envvar to be set in {RS_3_2, RS_6_3, RS_10_4}
    * SYCL_IP_NAME as a string
2. It requires ISA-L to be installed in the system
3. It adds a new make target, `plain_c`: Build the sources as a C application, without any SYCL feature, for faster debug

## ASP-based Variants
For ASP-compatible image:
* It requires CMake flags: `-DIS_BSP=1 -DIS_USM=1` at **configuration-time**.
* It offers a zero-copy variant, if ASP_ZERO_COPY matches 1 at **build-time**.

## Multi-erasure Variants
There are two variants for the host source file, selected at **build-time** based on the definition of `MULTI_ERASURE_SIMPLE` envvar:
    * `src/host_one_erasure.cpp`, if not defined.
    * `src/host_multi_erasure.cpp`, if defined.

Import `src/host_multi_erasure.cpp` with:
``` console
$ make <target> CXX_DEFINES="-DMULTI_ERASURE_SIMPLE"
```

## Debug
Enable debug prints with:
``` console
$ make <target> CXX_DEFINES="... -DDEBUG"
```

## IP Authoring Flow
To generate the kernel IP sources, just run `make report` and export the sources from `${SYCL_IP_NAME}_report.prj`.

## Sources
```
├── CMakeLists.txt
├── host.cpp
├── host_multi_erasure.cpp
├── measure_latency.h
└── rs_erasure
    ├── roms                # RS ROM generation subproject
    |   └── ...
    ├── rs_erasure.cpp      # Kernel code
    ├── rs_erasure.hpp      # Non-SYCL header file
    └── rs_erasure_sycl.hpp # SYCL-only sources
```
> NOTE: the `roms` subproject is going to be exported in a separate repo.