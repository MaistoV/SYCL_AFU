#ifndef _OPAE_SIMPLE_WRAPPER_
#define _OPAE_SIMPLE_WRAPPER_

#include <opae/fpga.h> 	// for opae types
#include <uuid/uuid.h> 	// for uuid_parse()
#include <stdio.h>		// for fprintf()
#include <stdlib.h> 	// for malloc
#include <unistd.h> 	// for usleep
#include <assert.h> 	// for assert()
#include <poll.h> 		// For poll()
#include <chrono>		// for measures
// #include <errno.h>

// Register map emitted for DFL
#include "afu_regmap.h"

// Width of MMIO interface in bytes
#define OSW_LINE_BYTE_WIDTH (64u)
// Milliseconds wait for poll()
#define POLL_TIMEOUT_MS 100
// Microseconds wait for AFU CSR polling
#define SLEEP_TIME_US 1

#define OSW_fpga_assert(res) if (FPGA_OK != (res)) { \
							printf("%s:%d %s\n", __FILE__, __LINE__, fpgaErrStr((res))); \
							exit((res)); \
						}

typedef struct OPAE_SIMPLE_WRAPPER_pcie_sbdf {
	uint16_t segment;
	uint8_t bus;
	uint8_t device;
	uint8_t function;
} OPAE_SIMPLE_WRAPPER_pcie_sbdf_t;


/// @brief Parse string in format "SSSS:BB:DD.F"
/// @param input_string Input string in expected format
/// @param pcie_sbdf Pointer to desination struct
/// @return 0 if ok, -1 if wrong format
/// @note This function uses strtol(), if chars in @input_string are not hex digits, 0 will be used instead
int OPAE_SIMPLE_WRAPPER_parse_pcie_sbdf ( 
									const char* input_string,
									OPAE_SIMPLE_WRAPPER_pcie_sbdf_t* pcie_sbdf
								);

/// @brief Helping function for MMIO writes, assuming ASE not supporting mapped MMIO access
/// @param accel_handle Handle to the AFU accelerator (only necessary for ASE)
/// @param mmio_num MMIO space number (only necessary for ASE)
/// @param mmio_ptr Pointer to mapped MMIO space
/// @param offset Offset in MMIO space
/// @param value 64-bit value to write
/// @return N/A
void OPAE_SIMPLE_WRAPPER_mmio64_write (
									fpga_handle 		accel_handle,
									uint32_t			mmio_num,
									volatile uint64_t * mmio_ptr,
									uint64_t			offset,
									uint64_t 			value
								);

/// @brief Helping function for MMIO writes, assuming ASE not supporting mapped MMIO access
/// @param accel_handle Handle to the AFU accelerator (only necessary for ASE)
/// @param mmio_num MMIO space number (only necessary for ASE)
/// @param mmio_ptr Pointer to mapped MMIO space
/// @param offset Offset in MMIO space
/// @param value 32-bit value to write
/// @return N/A
void OPAE_SIMPLE_WRAPPER_mmio32_write (
					fpga_handle 		accel_handle,
					uint32_t			mmio_num,
					volatile uint64_t * mmio_ptr,
					uint64_t			offset,
					uint32_t 			value
				);

/// @brief Helping function for MMIO reads, assuming ASE not supporting mapped MMIO access
/// @param accel_handle Handle to the AFU accelerator (only necessary for ASE)
/// @param mmio_num MMIO space number (only necessary for ASE)
/// @param mmio_ptr Pointer to mapped MMIO space
/// @param offset Offset in MMIO space
/// @param dest Reference to destination variable
/// @return N/A
void OPAE_SIMPLE_WRAPPER_mmio64_read (
					fpga_handle 		accel_handle,
					uint32_t			mmio_num,
					volatile uint64_t * mmio_ptr,
					uint64_t			offset,
					uint64_t * 			dest
				);

/// @brief Grub and initialize FPGA AFU
/// @param[out] accel_handle Handle to the AFU accelerator
/// @param[in]  accel_uuid UUID of the target AFU
/// @param[out] mmio_ptr Pointer to mapped MMIO space
/// @param[in]  pcie_sbdf Struct for PCIe S:B:D:F function ID
/// @param[out] Reference to mmio_num MMIO space number
/// @return fpga_result-encoded exit code
fpga_result OPAE_SIMPLE_WRAPPER_init ( 
							fpga_handle * 					accel_handle,
							const char *					accel_uuid,
                           	volatile uint64_t **			mmio_ptr,
							OPAE_SIMPLE_WRAPPER_pcie_sbdf_t	pcie_sbdf,
							uint32_t *						mmio_num
						);

/// @brief Allocate the input and output buffers for AFU
/// @param[in]  accel_handle Handle to the AFU accelerator
/// @param[in]  size Requested size for buffer 
/// @param[out] io_addr Pointer to memory where the address will be returned
/// @param[out] wsid Buffer handle / workspace ID  (for later OPAE_SIMPLE_WRAPPER_cleanup)
/// @return host-side pointer
volatile void * OPAE_SIMPLE_WRAPPER_allocate_io_buffer (
												fpga_handle accel_handle,
												ssize_t 	size,
												uint64_t *	wsid,
												uint64_t *	io_addr
											);

/// @brief Load AFU parameters, start AFU, wait for interrupt with poll, check poll results
/// @param[in]  accel_handle Handle to the AFU accelerator
/// @param[in]  erasure_pattern One-hot bitstring for erased cell/block
/// @param[in]  survived_cells Bitstring for survived cells/blocks
/// @param[in]  cell_length Cell/block length in bytes
/// @param[in]  sleep_time_us Time interval in microseconds for polling BUSY register
/// @param[out] fpgaInterruptEvent Event handle for interrupts (for later OPAE_SIMPLE_WRAPPER_cleanup)
/// @param[in]  measure_latency Measure latency or not, and dump it on file
/// @param[in]  mmio_ptr Pointer to mapped MMIO space
/// @param[in]  mmio_num MMIO space number
/// @param[in]  fd_latency File descriptor of an opened file for latency measuring, needed only if measure_latency is true
/// @return fpga_result-encoded exit code
fpga_result OPAE_SIMPLE_WRAPPER_call_afu (
                                fpga_handle 		accel_handle, 
                                uint16_t			erasure_pattern,	 	
                                uint16_t			survived_cells,
                                uint32_t			cell_length,
                                uint32_t 			sleep_time_us,
                                fpga_event_handle *	fpgaInterruptEvent,
                                uint32_t			measure_latency,
                                volatile uint64_t *	mmio_ptr,
								uint32_t			mmio_num,
                                FILE*       		fd_latency
                            );

/// @brief Clean-up OPAE structures
/// @param[in]  accel_handle Handle to the AFU accelerator
/// @param[in]  mmio_num MMIO space number
/// @param[in]  fpgaInterruptEvent Event handle for interrupts (from OPAE_SIMPLE_WRAPPER_call_afu)
/// @param[in]  workspace_id_in  Buffer handle / workspace ID for input (from OPAE_SIMPLE_WRAPPER_allocate_io_buffers)
/// @param[in]  workspace_id_out  Buffer handle / workspace ID for output (from OPAE_SIMPLE_WRAPPER_allocate_io_buffers)
/// @return fpga_result-encoded exit code
fpga_result OPAE_SIMPLE_WRAPPER_cleanup (
                                fpga_handle 		accel_handle,
								uint32_t			mmio_num,
                                fpga_event_handle *	fpgaInterruptEvent,
                                uint64_t 			input_buf_workspace_id,
                                uint64_t 			output_buf_workspace_id
                            );

/// @brief Read AFU debug registers thorugh MMIO
/// @param[in]  accel_handle Handle to the AFU accelerator
/// @param[in]  mmio_num MMIO space number
/// @param[in]  mmio_ptr Pointer to mapped MMIO space
/// @return fpga_result-encoded exit code
fpga_result OPAE_SIMPLE_WRAPPER_debug_read ( 
                                fpga_handle 		accel_handle,
								uint32_t			mmio_num,
								volatile uint64_t *	mmio_ptr
                            );


#endif // _OPAE_SIMPLE_WRAPPER_