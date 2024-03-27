#ifndef __SYCL_REGISTER_MAP_OFFSET_H__
#define __SYCL_REGISTER_MAP_OFFSET_H__

// Register map emitted by SYCL
#include "register_map_offsets.hpp"

// Register map of dfl_csr_avalon_proxy
#include "dfl_csr_regmap.h"

// Patched register map
#define KERNEL_STATUS                   (ZTS14SYCLLOOPBACKID_REGISTER_MAP_STATUS_REG)   // Status register
// Missing in register_map_offsets.hpp
#define KERNEL_START                    (0x8 + ZTS14SYCLLOOPBACKID_REGISTER_MAP_OFFSET) // Start the kernel, don't read in ASE! 
#define KERNEL_FINISH_COUNTER           (ZTS14SYCLLOOPBACKID_REGISTER_MAP_FINISHCOUNTER_REG) // Get the number of finish runs, this will also clear the register and pending interrupt
#define KERNEL_CLEAR_INTERRUPT          (ZTS14SYCLLOOPBACKID_REGISTER_MAP_FINISHCOUNTER_REG) // Read to clear pending interrupt, alias for KERNEL_FINISH_COUNTER
#define KERNEL_ARG_DEVICE_READ_REG      (ZTS14SYCLLOOPBACKID_REGISTER_MAP_ARG_ARG_DEVICE_READ_REG)  // Read interface
#define KERNEL_ARG_DEVICE_WRITE_REG     (ZTS14SYCLLOOPBACKID_REGISTER_MAP_ARG_ARG_DEVICE_WRITE_REG) // Write interface
#define KERNEL_ARG_RS_LENGTH_LINES_REG  (ZTS14SYCLLOOPBACKID_REGISTER_MAP_ARG_ARG_LENGTH_LINES_REG) // Number of cache lines to read/write

// Values
#define KERNEL_START_VALUE              ((uint32_t)1u) // This must be 32-bits log

#endif // __SYCL_REGISTER_MAP_OFFSET_H__
