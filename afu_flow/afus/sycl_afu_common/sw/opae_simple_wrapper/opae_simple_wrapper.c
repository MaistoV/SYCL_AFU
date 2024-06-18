#include "opae_simple_wrapper.h"

void OPAE_SIMPLE_WRAPPER_mmio64_write (
									fpga_handle 		accel_handle,
									uint32_t			mmio_num,
									volatile uint64_t * mmio_ptr,
									uint64_t			offset,
									uint64_t 			value
								) {
#ifndef NO_ASE_SUPPORT
	if ( getenv("WITH_ASE") != NULL ) {
        fpga_result res = fpgaWriteMMIO64(accel_handle, mmio_num, offset, value);
		fpga_assert(res);
	}
	else
#endif // !NO_ASE_SUPPORT
		MAPPED_MMIO(mmio_ptr, offset) = value;
}

void OPAE_SIMPLE_WRAPPER_mmio32_write (
                                    fpga_handle         accel_handle,
                                    uint32_t            mmio_num,
                                    volatile uint64_t * mmio_ptr,
                                    uint64_t            offset,
                                    uint64_t             value
                                ) {
#ifndef NO_ASE_SUPPORT
	if ( getenv("WITH_ASE") != NULL ) {
        fpga_result res = fpgaWriteMMIO32(accel_handle, mmio_num, offset, value);
		fpga_assert(res);
	}
	else
#endif // !NO_ASE_SUPPORT
		MAPPED_MMIO(mmio_ptr, offset) = value;
}


void OPAE_SIMPLE_WRAPPER_mmio64_read (
					fpga_handle 		accel_handle,
					uint32_t			mmio_num,
					volatile uint64_t * mmio_ptr,
					uint64_t			offset,
					uint64_t * 			dest
				) {
#ifndef NO_ASE_SUPPORT
	if ( getenv("WITH_ASE") != NULL ) {
        fpga_result res = fpgaReadMMIO64(accel_handle, mmio_num, offset, dest);
		fpga_assert(res);
	}
	else
#endif // !NO_ASE_SUPPORT
		*dest = MAPPED_MMIO(mmio_ptr, offset);
}

fpga_result OPAE_SIMPLE_WRAPPER_debug_read ( 
                                fpga_handle         accel_handle,
                                uint32_t            mmio_num,
                                volatile uint64_t * mmio_ptr
                            ) {
	fpga_result res = FPGA_OK;

	// MMIO DFL CSRs access
	uint64_t data = 0;
	OPAE_SIMPLE_WRAPPER_mmio64_read ( accel_handle, mmio_num, mmio_ptr, AFU_DFH_REG, &data );
	printf("AFU_DFH_REG = %016lx\n", data);
	OPAE_SIMPLE_WRAPPER_mmio64_read ( accel_handle, mmio_num, mmio_ptr, AFU_ID_LO, &data );
	printf("AFU ID LO = %016lx\n", data);
	OPAE_SIMPLE_WRAPPER_mmio64_read ( accel_handle, mmio_num, mmio_ptr, AFU_ID_HI, &data );
	printf("AFU ID HI = %016lx\n", data);
	OPAE_SIMPLE_WRAPPER_mmio64_read ( accel_handle, mmio_num, mmio_ptr, AFU_NEXT, &data );
	printf("AFU NEXT = %016lx\n", data);
	OPAE_SIMPLE_WRAPPER_mmio64_read ( accel_handle, mmio_num, mmio_ptr, AFU_RESERVED, &data );
	printf("AFU RESERVED = %016lx\n", data);

	return res;
}

// Parse string in format "SSSS:BB:DD.F"
int OPAE_SIMPLE_WRAPPER_parse_pcie_sbdf ( 
                                    const char* input_string,
                                    OPAE_SIMPLE_WRAPPER_pcie_sbdf_t* pcie_sbdf
                                ) {
	// Return value
	(*pcie_sbdf) = {
								.segment 	= 0,
								.bus 		= 1,
								.device 	= 0,
								.function 	= 1
							};

	// Utility string
	char tmp_string[5];
	// Assume 12 chars
	char sbdf_string[] = "SSSS:BB:DD.F";;
	const size_t len_sbdf = strlen(sbdf_string);

	// Copy input to local string
	strncpy(sbdf_string, input_string, len_sbdf);
	sbdf_string[len_sbdf] = '\0';

	// Check valid PCIe format, expected SSSS:BB:DD.F"
	// TODO: check values are valid hex digits, otherwise 0 is returned from strtol()
	if (
		(sbdf_string[ 4] != ':') ||
		(sbdf_string[ 7] != ':') ||
		(sbdf_string[10] != '.') 
	) {
			return -1;
	}

	// Parse SSSS
	strncpy(tmp_string, sbdf_string, 4);
	tmp_string[4] = '\0';
	(*pcie_sbdf).segment	= strtol(tmp_string, NULL, 16);
	// Parse BB
	strncpy(tmp_string, &(sbdf_string[5]), 2);
	tmp_string[2] = '\0';
	(*pcie_sbdf).bus 		= strtol(tmp_string, NULL, 16);
	// Parse DD
	strncpy(tmp_string, &(sbdf_string[8]), 2);
	tmp_string[2] = '\0';
	(*pcie_sbdf).device	= strtol(tmp_string, NULL, 16);
	// Parse F
	strncpy(tmp_string,  &(sbdf_string[11]), 1);
	tmp_string[1] = '\0';
	(*pcie_sbdf).function	= strtol(tmp_string, NULL, 16);

	// Ok
	return 0;
}

fpga_result OPAE_SIMPLE_WRAPPER_init ( 
                            fpga_handle *                   accel_handle,
                            const char *                    accel_uuid,
                            volatile uint64_t **            mmio_ptr,
                            OPAE_SIMPLE_WRAPPER_pcie_sbdf_t	pcie_sbdf,
                            uint32_t *                      mmio_num
						) {
    fpga_result res = FPGA_OK;
	
	if ( accel_handle == NULL ) {									
		return FPGA_INVALID_PARAM;
	}
	
	// Compose the filter object
	fpga_properties filter = NULL;
    res = fpgaGetProperties(NULL, &filter);
	OSW_fpga_assert(res);
    res = fpgaPropertiesSetObjectType(filter, FPGA_ACCELERATOR);
	OSW_fpga_assert(res);

    // Add the desired UUID to the filter
    fpga_guid guid;
	if (uuid_parse(accel_uuid, guid) < 0) {
		fprintf(stderr, "Error parsing guid '%s'\n", accel_uuid);
		return FPGA_INVALID_PARAM;
	}	
	res = fpgaPropertiesSetGUID(filter, guid);
	OSW_fpga_assert(res);

#ifdef DEBUG_OSW
	printf("%s:%d: PCIe address %04x:%02x:%02x.%01x\n",
			__FILE__, __LINE__,
			pcie_sbdf.segment,
			pcie_sbdf.bus,
			pcie_sbdf.device,
			pcie_sbdf.function
		);
#endif // DEBUG_OSW

	// Add PCIe S:B:D:F to filter
	res = fpgaPropertiesSetSegment	(filter, pcie_sbdf.segment	);
	OSW_fpga_assert(res);
	res = fpgaPropertiesSetBus		(filter, pcie_sbdf.bus		);
	OSW_fpga_assert(res);
	res = fpgaPropertiesSetDevice	(filter, pcie_sbdf.device	);
	OSW_fpga_assert(res);
	res = fpgaPropertiesSetFunction	(filter, pcie_sbdf.function	);
	OSW_fpga_assert(res);

    // Enumerate and get the tokens
    uint32_t num_matches;
	const uint32_t max_tokens = 1; // We need just one
    fpga_token accel_token;
    res = fpgaEnumerate(&filter, 1, &accel_token, max_tokens, &num_matches);
	OSW_fpga_assert(res);
    if ( num_matches < 1 ) {
        fprintf(stderr, "%s:%d: PCIe address %04x:%02x:%02x.%01x\n AFU %s not found!\n",
			__FILE__, __LINE__, 
			pcie_sbdf.segment,
			pcie_sbdf.bus,
			pcie_sbdf.device,
			pcie_sbdf.function,
			accel_uuid
			);
		return FPGA_INVALID_PARAM;
	}

    // Open match
	res = fpgaOpen(accel_token, accel_handle, 0);
	OSW_fpga_assert(res);

	// Map MMIO address space
	// Not supported by ASE
	if ( getenv("WITH_ASE") == NULL ) {
		volatile uint64_t * tmp_ptr;
		*mmio_num = (pcie_sbdf.device << 3) + pcie_sbdf.function;
		res = fpgaMapMMIO(*accel_handle, *mmio_num, ((uint64_t **)&tmp_ptr));
		fpga_assert(res);
		assert(tmp_ptr != NULL);
		*mmio_ptr = tmp_ptr;
	}

	// AFU reset via CSR
	OPAE_SIMPLE_WRAPPER_mmio64_write ( *accel_handle, *mmio_num, *mmio_ptr, AFU_RESET, AFU_RESET_VALUE );
	OPAE_SIMPLE_WRAPPER_mmio64_write ( *accel_handle, *mmio_num, *mmio_ptr, AFU_IRQ_EN, AFU_IRQ_EN_VALUE );

#ifdef DEBUG_OSW
	res = OPAE_SIMPLE_WRAPPER_debug_read( *accel_handle, *mmio_ptr );
#endif // DEBUG_OSW

	// Reset AFU
	// Not supported by vfio plugin
	// res = fpgaReset( *accel_handle );
	// OSW_fpga_assert(res);
	
    // Clean up
    res = fpgaDestroyProperties(&filter);
    OSW_fpga_assert(res);
	res = fpgaDestroyToken(&accel_token);
	OSW_fpga_assert(res);

    return res;
}

volatile void * OPAE_SIMPLE_WRAPPER_allocate_io_buffer (
												fpga_handle	accel_handle,
                                                ssize_t     size,
                                                uint64_t *  wsid,
                                                uint64_t *  io_addr
											) {
    fpga_result res;
    volatile void* buf;

    int flags = 0;
    res = fpgaPrepareBuffer(accel_handle, size, (void**)&buf, wsid, flags);
    OSW_fpga_assert(res);

    // Get the physical address of the buffer for the accelerator
    res = fpgaGetIOAddress(accel_handle, *wsid, io_addr);
    OSW_fpga_assert(res);

#ifdef DEBUG_OSW
	printf("%s:%d io_addr %016lx:\n", __FILE__, __LINE__, *io_addr );
    printf("%s:%d buf %p:\n", __FILE__, __LINE__, buf );
#endif // DEBUG_OSW

    return buf;
}

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
                            ) {
	fpga_result res = FPGA_OK;
	int poll_res;
	struct pollfd pfd;
	// For measures
	double time_sec = 0.;
    std::chrono::time_point<
		std::chrono::steady_clock,
		std::chrono::nanoseconds
		> start, end;

	if ( accel_handle == NULL ) {									
		return FPGA_INVALID_PARAM;
	}

	////////////////////////////////
	// Load AFU parameters
	////////////////////////////////
	// Serialize CSR inputs
	uint64_t rs_erasure_csrs = 0;
	uint64_t erasure_pattern_64 	= erasure_pattern;
	uint64_t survived_cells_64 		= survived_cells;
	uint64_t cell_length_64			= cell_length / OSW_LINE_BYTE_WIDTH;
	rs_erasure_csrs |= erasure_pattern_64 	<< 0u ;
	rs_erasure_csrs |= survived_cells_64 	<< 16u;
	rs_erasure_csrs |= cell_length_64		<< 32u;

	OPAE_SIMPLE_WRAPPER_mmio64_write ( accel_handle, mmio_num, mmio_ptr, KERNEL_ARG_RS_ERASURE_CSR_REG, rs_erasure_csrs );
#ifdef DEBUG_OSW
	printf("%s:%d erasure_pattern_64  0x%016lx\n", __FILE__, __LINE__, erasure_pattern_64);
	printf("%s:%d survived_cells_64   0x%016lx\n", __FILE__, __LINE__, survived_cells_64);
	printf("%s:%d cell_length_64      %lu\n", __FILE__, __LINE__, cell_length_64);
	printf("%s:%d write @%x, value = %lx\n", __FILE__, __LINE__, KERNEL_ARG_RS_ERASURE_CSR_REG, rs_erasure_csrs);
#endif // DEBUG_OSW

	////////////////////////////////
	// Create event for interrupt //
	////////////////////////////////
#ifdef INTERRUPT_EVENTS
	// Register user interrupt with event accel_handle
	res = fpgaCreateEventHandle(fpgaInterruptEvent);
	OSW_fpga_assert(res);
	uint32_t flags = 0; // uses IRQ bit 0, see instantiation of acmm_ccip_host_wr in afu.sv
	res = fpgaRegisterEvent(accel_handle, FPGA_EVENT_INTERRUPT, *fpgaInterruptEvent, flags);
	OSW_fpga_assert(res);
#endif // INTERRUPT_EVENTS

	//////////////////////////////
	// Wait for AFU to be ready //
	//////////////////////////////
	// Poll on busy register
	// TODO: is there a cleaner way?
	uint64_t status_val;
	do {
		sleep( SLEEP_TIME_US );
		OPAE_SIMPLE_WRAPPER_mmio64_read ( accel_handle, mmio_num, mmio_ptr, KERNEL_STATUS, &status_val );
	#ifdef DEBUG_OSW
		print_kernel_status(status_val);
	#endif // DEBUG_OSW
	} while ( status_val & KERNEL_REGISTER_MAP_BUSY_MASK );

	////////////////////////
	// Start and wait AFU //
	////////////////////////
	// fpga_result start_and_wait_afu(fpga_handle afc_handle, struct pollfd *pfd, int *poll_res)
#ifdef INTERRUPT_EVENTS
	pfd.events = POLLIN;
	res = fpgaGetOSObjectFromEventHandle(*fpgaInterruptEvent, &pfd.fd);
	OSW_fpga_assert(res);
#endif // INTERRUPT_EVENTS

	// Start measure by macro
	if ( measure_latency ) {
		// MEASURE_LATENCY_START(start);
		// Expand macro to reduce dependecies
		start = std::chrono::steady_clock::now();
	}

	// Start the AFU by writing a '1' into the start register
	OPAE_SIMPLE_WRAPPER_mmio32_write(accel_handle, mmio_num, mmio_ptr, KERNEL_START, KERNEL_START_VALUE);
#ifdef DEBUG_OSW
	printf("%s:%d write @%x, value = %x\n", __FILE__, __LINE__, KERNEL_START, KERNEL_START_VALUE);
#endif // DEBUG_OSW

#ifdef INTERRUPT_EVENTS
	// Wait for interrupt with poll()
	// NOTE: the current Linux driver implementation of poll()
	// has a minimum latency of 0.1 seconds
	poll_res = poll(&pfd, 1, POLL_TIMEOUT_MS);
#ifdef DEBUG_OSW
	printf("%s:%d poll_res = %d\n", __FILE__, __LINE__, poll_res);
	// Check poll errors
	if ( poll_res <= 0 ) {
		fprintf(stderr, "Poll error errno = %s\n", strerror(errno));
		return FPGA_EXCEPTION;
	}
	else if ( poll_res == 0 ) {
		fprintf(stderr, "Error: Poll timeout \n");
		return FPGA_EXCEPTION;
	}
	else {
		printf("Poll success. Return = %d\n", poll_res);
	}
#endif // DEBUG_OSW
#else // !INTERRUPT_EVENTS
	// Active polling on AFU
	do {
		usleep( SLEEP_TIME_US );
		OPAE_SIMPLE_WRAPPER_mmio64_read ( accel_handle, mmio_num, mmio_ptr, KERNEL_STATUS, &status_val );
	#ifdef DEBUG_OSW
		print_kernel_status(status_val);
	#endif // DEBUG_OSW
	} while ( !(status_val & KERNEL_REGISTER_MAP_DONE_MASK) );
#endif // INTERRUPT_EVENTS

	// End measure by macro
	if ( measure_latency ) {
		// MEASURE_LATENCY_END_AND_PRINT(start, time_sec, fd_latency);
		// Expand macro to reduce dependecies
		end = std::chrono::steady_clock::now();
		time_sec = std::chrono::duration_cast<std::chrono::duration<double>>(end - start).count();
		fprintf(fd_latency, "%0.10f\n", time_sec);
	}

	// Clear interrupt
	// NOTE: this is necessary across calls regardless of INTERRUPT_EVENTS
	OPAE_SIMPLE_WRAPPER_mmio64_read ( accel_handle, mmio_num, mmio_ptr, KERNEL_CLEAR_INTERRUPT, &status_val );
#ifdef DEBUG_OSW
	printf("%s:%d Read from KERNEL_CLEAR_INTERRUPT...\n", __FILE__, __LINE__);
	print_kernel_status(status_val);
#endif // DEBUG_OSW

	return res;
}

fpga_result OPAE_SIMPLE_WRAPPER_cleanup  (
                                fpga_handle 		accel_handle,
								uint32_t			mmio_num,
                                fpga_event_handle *	fpgaInterruptEvent,
                                uint64_t 			input_buf_workspace_id,
                                uint64_t 			output_buf_workspace_id
                            ) {	
	fpga_result res = FPGA_OK;

	if ( accel_handle == NULL ) {									
		return FPGA_INVALID_PARAM;
	}

#ifdef INTERRUPT_EVENTS
	// Cleanup event accel_handle			
	if ( fpgaInterruptEvent != NULL ) {									
		res = fpgaUnregisterEvent(accel_handle, FPGA_EVENT_INTERRUPT, *fpgaInterruptEvent);
		res = fpgaDestroyEventHandle(fpgaInterruptEvent);
	}
#endif // INTERRUPT_EVENTS

	// Release I/O buffers
	res = fpgaReleaseBuffer(accel_handle, output_buf_workspace_id);
	res = fpgaReleaseBuffer(accel_handle, input_buf_workspace_id);

	// Unmap MMIO space 
	res = fpgaUnmapMMIO(accel_handle, mmio_num); // Actually does nothing in vfio plugin

	// Release accelerator 
	res = fpgaClose(accel_handle);

	return res;
}