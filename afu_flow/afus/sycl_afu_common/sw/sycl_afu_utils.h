#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdio.h> // for printf()
#include <opae/fpga.h> // for fpga_handle
#include <assert.h> // for assert()
#include <uuid/uuid.h>  // for uuid_parse

#include "dfl_csr_regmap.h"

#define print_kernel_status( status_val ) \
    printf("%s:%d status_val = %0lx\n" , __FILE__, __LINE__ , status_val); \
    printf("\t.done    = %lx\n", (status_val & KERNEL_REGISTER_MAP_DONE_MASK    ) >> KERNEL_REGISTER_MAP_DONE_OFFSET      ); \
    printf("\t.busy    = %lx\n", (status_val & KERNEL_REGISTER_MAP_BUSY_MASK    ) >> KERNEL_REGISTER_MAP_BUSY_OFFSET      ); \
    printf("\t.stalled = %lx\n", (status_val & KERNEL_REGISTER_MAP_STALLED_MASK ) >> KERNEL_REGISTER_MAP_STALLED_OFFSET   ); \
    printf("\t.unstall = %lx\n", (status_val & KERNEL_REGISTER_MAP_UNSTALL_MASK ) >> KERNEL_REGISTER_MAP_UNSTALL_OFFSET   ); \
    printf("\t.valid   = %lx\n", (status_val & KERNEL_REGISTER_MAP_VALID_IN_MASK) >> KERNEL_REGISTER_MAP_VALID_IN_OFFSET  ); \
    printf("\t.started = %lx\n", (status_val & KERNEL_REGISTER_MAP_STARTED_MASK ) >> KERNEL_REGISTER_MAP_STARTED_OFFSET   );

#define fpga_assert(res) if (FPGA_OK != (res)) { \
                            printf("%s:%d %s\n", __FILE__, __LINE__, fpgaErrStr((res))); \
                            exit((res)); \
                        }

// Utility functions

// Search for all accelerators matching the requested properties and
// connect to them. The input value of *num_handles is the maximum
// number of connections allowed. (The size of accel_handles.) The
// output value of *num_handles is the actual number of connections.
fpga_result connect_to_matching_accels(
                           const char *accel_uuid,
                           uint32_t *num_handles,
                           fpga_handle *accel_handles,
                           bool *is_ase_sim,
                           uint64_t** ptr_mmio
                           );

// Allocate a buffer in I/O memory, shared with the FPGA.
volatile void* alloc_buffer(fpga_handle accel_handle,
            ssize_t size,
            uint64_t *wsid,
            uint64_t *io_addr);

               
// Debug reads from DFL and Kernel CSRs
void debug_read_dfl( fpga_handle accel_handle );

#endif // __UTILS_H__

