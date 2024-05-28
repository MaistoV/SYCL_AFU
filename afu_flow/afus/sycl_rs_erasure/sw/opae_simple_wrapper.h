#ifndef _OPAE_SIMPLE_WRAPPER_
#define _OPAE_SIMPLE_WRAPPER_

#define POLL_TIMEOUT_MS 100
// Microseconds wait for AFU CSR polling
#define SLEEP_TIME_US 1

/// @brief Helping function for MMIO writes, assuming ASE not supporting mapped MMIO access
/// @param accel_handle Handle to the AFU accelerator
/// @param mmio_ptr Pointer to mapped MMIO space
/// @param offset Offset in MMIO space
/// @param value 64-bit value to write
/// @return N/A
void OPAE_SIMPLE_WRAPPER_mmio64_write (
					fpga_handle 		accel_handle,
					volatile uint64_t * mmio_ptr,
					uint64_t			offset,
					uint64_t 			value
				);

/// @brief Helping function for MMIO writes, assuming ASE not supporting mapped MMIO access
/// @param accel_handle Handle to the AFU accelerator
/// @param mmio_ptr Pointer to mapped MMIO space
/// @param offset Offset in MMIO space
/// @param value 32-bit value to write
/// @return N/A
void OPAE_SIMPLE_WRAPPER_mmio32_write (
					fpga_handle 		accel_handle,
					volatile uint64_t * mmio_ptr,
					uint64_t			offset,
					uint32_t 			value
				);

/// @brief Helping function for MMIO reads, assuming ASE not supporting mapped MMIO access
/// @param accel_handle Handle to the AFU accelerator
/// @param mmio_ptr Pointer to mapped MMIO space
/// @param offset Offset in MMIO space
/// @param dest Reference to destination variable
/// @return N/A
void OPAE_SIMPLE_WRAPPER_mmio64_read (
					fpga_handle 		accel_handle,
					volatile uint64_t * mmio_ptr,
					uint64_t			offset,
					uint64_t * 			dest
				);

/// @brief Grub and initialize FPGA AFU
/// @param accel_handle Handle to the AFU accelerator
/// @param accel_uuid UUID of the target AFU
/// @param mmio_ptr Pointer to mapped MMIO space
/// @return fpga_result-encoded exit code
fpga_result OPAE_SIMPLE_WRAPPER_init ( 
							fpga_handle* accel_handle,
							const char *accel_uuid,
                           	volatile uint64_t** mmio_ptr
						);

/// @brief Allocate the input and output buffers for AFU
/// @param accel_handle Handle to the AFU accelerator
/// @param size Requested size for buffer 
/// @param io_addr Pointer to memory where the address will be returned
/// @param wsid Buffer handle / workspace ID  (for later OPAE_SIMPLE_WRAPPER_cleanup)
/// @return host-side pointer
volatile void * OPAE_SIMPLE_WRAPPER_allocate_io_buffer (
												fpga_handle accel_handle,
												ssize_t size,
												uint64_t *wsid,
												uint64_t *io_addr
											);

/// @brief Load AFU parameters, start AFU, wait for interrupt with poll, check poll results
/// @param accel_handle Handle to the AFU accelerator
/// @param erasure_pattern One-hot bitstring for erased cell/block
/// @param survived_cells Bitstring for survived cells/blocks
/// @param cell_length Cell/block length in bytes
/// @param sleep_time_us Time interval in microseconds for polling BUSY register
/// @param fpgaInterruptEvent Event handle for interrupts (for later OPAE_SIMPLE_WRAPPER_cleanup)
/// @param fd_latency File descriptor of an opened file for latency measuring, needed only if MEASURE_LATENCY is defined
/// @return fpga_result-encoded exit code
fpga_result OPAE_SIMPLE_WRAPPER_call_afu (
                                fpga_handle accel_handle, 
                                uint16_t	erasure_pattern,	 	
                                uint16_t	survived_cells,
                                uint32_t	cell_length,
                                uint32_t 	sleep_time_us,
                                fpga_event_handle *fpgaInterruptEvent,
                                uint32_t	measure_latency,
                                volatile uint64_t * mmio_ptr,
                                FILE*       fd_latency
                            );

/// @brief Clean-up OPAE structures
/// @param accel_handle Handle to the AFU accelerator
/// @param fpgaInterruptEvent Event handle for interrupts (from OPAE_SIMPLE_WRAPPER_call_afu)
/// @param workspace_id_in  Buffer handle / workspace ID for input (from OPAE_SIMPLE_WRAPPER_allocate_io_buffers)
/// @param workspace_id_out  Buffer handle / workspace ID for output (from OPAE_SIMPLE_WRAPPER_allocate_io_buffers)
/// @return fpga_result-encoded exit code
fpga_result OPAE_SIMPLE_WRAPPER_cleanup (
                                fpga_handle accel_handle,
                                fpga_event_handle *fpgaInterruptEvent,
                                uint64_t input_buf_workspace_id,
                                uint64_t output_buf_workspace_id
                            );

/// @brief Read AFU debug registers thorugh MMIO
/// @param accel_handle Handle to the AFU accelerator
/// @param mmio_ptr Pointer to mapped MMIO space
/// @return fpga_result-encoded exit code
fpga_result OPAE_SIMPLE_WRAPPER_debug_read ( 
                                fpga_handle accel_handle,
                                volatile uint64_t* mmio_ptr
                            );


#endif // _OPAE_SIMPLE_WRAPPER_