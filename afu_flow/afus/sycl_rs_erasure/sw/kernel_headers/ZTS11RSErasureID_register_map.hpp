
/* This header file describes the Register Map for the ZTS11RSErasureID kernel */

/* Note that this header file should NOT be included directly! */
/* Please include the top-level header file register_map_offsets.hpp instead! */

#ifndef __ZTS11RSERASUREID_REGISTER_MAP_REGS_H__
#define __ZTS11RSERASUREID_REGISTER_MAP_REGS_H__



/* Status register contains all the control bits to control kernel execution */
/******************************************************************************/
/* Memory Map Summary                                                         */
/******************************************************************************/

/*
 Address | Access | Register     | Argument                            | Description 
---------|--------|--------------|-------------------------------------|-------------------------------
     0x0 |    R/W |   reg0[63:0] |                        Status[63:0] |   * Read/Write the status bits
         |        |              |                                     |       that are described below
---------|--------|--------------|-------------------------------------|-------------------------------
     0x8 |    R/W |   reg1[31:0] |                         Start[31:0] |        * Write 1 to initiate a
         |        |              |                                     |                   kernel start
---------|--------|--------------|-------------------------------------|-------------------------------
    0x30 |      R |   reg6[31:0] |                 FinishCounter[31:0] | * Read to get number of kernel
         |        |  reg6[63:32] |                 FinishCounter[31:0] |       finishes, note that this
         |        |              |                                     |    register will clear on read
---------|--------|--------------|-------------------------------------|-------------------------------
    0x80 |      W |  reg16[63:0] |               arg_device_read[63:0] |                              
---------|--------|--------------|-------------------------------------|-------------------------------
    0x88 |      W |  reg17[63:0] |              arg_device_write[63:0] |                              
---------|--------|--------------|-------------------------------------|-------------------------------
    0x90 |      W |  reg18[63:0] |            arg_rs_erasure_csr[63:0] |                              
*/


/******************************************************************************/
/* Register Address Macros                                                    */
/******************************************************************************/

/* Status Register Bit Offsets (Bits) */
/* Note: Bits In Status Registers Are Marked As Read-Only or Read-Write
   Please Do Not Write To Read-Only Bits */
#ifndef __REGISTER_BITOFFSET_MACROS__
#define __REGISTER_BITOFFSET_MACROS__
#define KERNEL_REGISTER_MAP_DONE_OFFSET (1) // Read-only
#define KERNEL_REGISTER_MAP_BUSY_OFFSET (2) // Read-only
#define KERNEL_REGISTER_MAP_STALLED_OFFSET (3) // Read-only
#define KERNEL_REGISTER_MAP_UNSTALL_OFFSET (4) // Read-write
#define KERNEL_REGISTER_MAP_VALID_IN_OFFSET (14) // Read-only
#define KERNEL_REGISTER_MAP_STARTED_OFFSET (15) // Read-only
#endif

/* Status Register Bit Masks (Bits) */
#ifndef __REGISTER_BITMASK_MACROS__
#define __REGISTER_BITMASK_MACROS__
#define KERNEL_REGISTER_MAP_DONE_MASK (0x2)
#define KERNEL_REGISTER_MAP_BUSY_MASK (0x4)
#define KERNEL_REGISTER_MAP_STALLED_MASK (0x8)
#define KERNEL_REGISTER_MAP_UNSTALL_MASK (0x10)
#define KERNEL_REGISTER_MAP_VALID_IN_MASK (0x4000)
#define KERNEL_REGISTER_MAP_STARTED_MASK (0x8000)
#endif

/* Byte Addresses */
#define ZTS11RSERASUREID_REGISTER_MAP_STATUS_REG (0x0 + ZTS11RSERASUREID_REGISTER_MAP_OFFSET)
#define ZTS11RSERASUREID_REGISTER_MAP_START_REG (0x8 + ZTS11RSERASUREID_REGISTER_MAP_OFFSET)
#define ZTS11RSERASUREID_REGISTER_MAP_FINISHCOUNTER_REG (0x30 + ZTS11RSERASUREID_REGISTER_MAP_OFFSET)
#define ZTS11RSERASUREID_REGISTER_MAP_FINISHCOUNTER_REG (0x30 + ZTS11RSERASUREID_REGISTER_MAP_OFFSET)
#define ZTS11RSERASUREID_REGISTER_MAP_ARG_ARG_DEVICE_READ_REG (0x80 + ZTS11RSERASUREID_REGISTER_MAP_OFFSET)
#define ZTS11RSERASUREID_REGISTER_MAP_ARG_ARG_DEVICE_WRITE_REG (0x88 + ZTS11RSERASUREID_REGISTER_MAP_OFFSET)
#define ZTS11RSERASUREID_REGISTER_MAP_ARG_ARG_RS_ERASURE_CSR_REG (0x90 + ZTS11RSERASUREID_REGISTER_MAP_OFFSET)

/* Argument Sizes (bytes) */
#define ZTS11RSERASUREID_REGISTER_MAP_ARG_ARG_DEVICE_READ_SIZE (8)
#define ZTS11RSERASUREID_REGISTER_MAP_ARG_ARG_DEVICE_WRITE_SIZE (8)
#define ZTS11RSERASUREID_REGISTER_MAP_ARG_ARG_RS_ERASURE_CSR_SIZE (8)

/* Argument Masks */
#define ZTS11RSERASUREID_REGISTER_MAP_ARG_ARG_DEVICE_READ_MASK (0xffffffffffffffffULL)
#define ZTS11RSERASUREID_REGISTER_MAP_ARG_ARG_DEVICE_WRITE_MASK (0xffffffffffffffffULL)
#define ZTS11RSERASUREID_REGISTER_MAP_ARG_ARG_RS_ERASURE_CSR_MASK (0xffffffffffffffffULL)

#endif /* __ZTS11RSERASUREID_REGISTER_MAP_REGS_H__ */
