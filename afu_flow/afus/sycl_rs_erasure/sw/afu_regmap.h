#ifndef __SYCL_REGISTER_MAP_OFFSET_H__
#define __SYCL_REGISTER_MAP_OFFSET_H__

// Register map emitted by SYCL
#include "register_map_offsets.hpp"

// Register map of dfl_csr_avalon_proxy
#include "dfl_csr_regmap.h"

// Patched register map
#define KERNEL_STATUS                   (ZTS11RSERASUREID_REGISTER_MAP_STATUS_REG)                  // Status register
#define KERNEL_START                    (ZTS11RSERASUREID_REGISTER_MAP_START_REG)                   // Start the kernel
#define KERNEL_FINISH_COUNTER           (ZTS11RSERASUREID_REGISTER_MAP_FINISHCOUNTER_REG)           // Get the number of finish runs, this will also clear the register and pending interrupt
#define KERNEL_CLEAR_INTERRUPT          (ZTS11RSERASUREID_REGISTER_MAP_FINISHCOUNTER_REG)           // Read to clear pending interrupt, alias for KERNEL_FINISH_COUNTER
#define KERNEL_ARG_DEVICE_READ_REG      (ZTS11RSERASUREID_REGISTER_MAP_ARG_ARG_DEVICE_READ_REG)     // Read interface
#define KERNEL_ARG_DEVICE_WRITE_REG     (ZTS11RSERASUREID_REGISTER_MAP_ARG_ARG_DEVICE_WRITE_REG)    // Write interface
#define KERNEL_ARG_RS_ERASURE_CSR_REG   (ZTS11RSERASUREID_REGISTER_MAP_ARG_ARG_RS_ERASURE_CSR_REG)  // CSR word for rs_erasure

// Values
#define KERNEL_START_VALUE              ((uint32_t)1u) // This must be 32-bits log

#endif // __SYCL_REGISTER_MAP_OFFSET_H__
