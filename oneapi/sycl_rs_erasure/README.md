# SYCL ReedSolomon Kernel
These sources are meant for IP Authoring flow, nevertheless they are compatible with ASP runtime.

## Required Environment
Compared with the other SYCL exaples, the CMake flow defines more features:
1. It requires some envvars to be set:
    * RS_SCHEMA envvar to be set in {RS_3_2, RS_6_3, RS_10_4}
    * SYCL_IP_NAME as a string
    * MULTI_ERASURE_SIMPLE in {0,1}
    * ASP_ZERO_COPY in {0,1}
2. It requires ISA-L to be installed in the system
3. It adds a new make target, `plain_c`: Build the sources as a C application, without any SYCL feature, for faster debug

## ASP-based variants
For ASP-compatible image:
* It requires CMake flags: `-DIS_BSP=1 -DIS_USM=1`
* It offers a zero-copy variant, if ASP_ZERO_COPY matches 1
* It offers a multi-erasure variant, if MULTI_ERASURE_SIMPLE matches 1
    * `src/host_multi_erasure.cpp` gets imported instead of `src/host.cpp`

## IP Authoring flow
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