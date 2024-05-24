////////////////////////////////////////////////////////////////
// Some of this code is borrowed from opae_svc_wrapper.cpp
////////////////////////////////////////////////////////////////
#include <opae/fpga.h> // for opae types
#include <uuid/uuid.h> // for uuid_parse
#include <stdio.h>	// for fprintf
#include <stdlib.h> // for malloc
#include <unistd.h> // for usleep
#include <poll.h> // For poll()
// #include <errno.h>
#include <chrono>	// for measures

// Register map emitted for DFL
#include "afu_regmap.h"

// Measure macros for latency
#include "measure_latency.h"

// Utility functions
#include "sycl_afu_utils.h"

// RS header
#include "rs_erasure.hpp"

#include "opae_simple_wrapper.h"

// Helping function for MMIO writes, assuming ASE not supporting mapped MMIO access
void mmio64_write (
					fpga_handle 		accel_handle,
					volatile uint64_t * mmio_ptr,
					uint64_t			offset,
					uint64_t 			value
				) {
#ifndef NO_ASE_SUPPORT
	if ( getenv("WITH_ASE") != NULL ) {
        fpga_result res = fpgaWriteMMIO64(accel_handle, 0, offset, value);
		fpga_assert(res);
	}
	else
#endif // !NO_ASE_SUPPORT
		MAPPED_MMIO(mmio_ptr, offset) = value;
}

void mmio32_write (
					fpga_handle 		accel_handle,
					volatile uint64_t * mmio_ptr,
					uint64_t			offset,
					uint32_t 			value
				) {
#ifndef NO_ASE_SUPPORT
	if ( getenv("WITH_ASE") != NULL ) {
        fpga_result res = fpgaWriteMMIO32(accel_handle, 0, offset, value);
		fpga_assert(res);
	}
	else
#endif // !NO_ASE_SUPPORT
		MAPPED_MMIO(mmio_ptr, offset) = value;
}


// Helping function for MMIO reads, assuming ASE not supporting mapped MMIO access
void mmio64_read (
					fpga_handle 		accel_handle,
					volatile uint64_t * mmio_ptr,
					uint64_t			offset,
					uint64_t * 			dest
				) {
#ifndef NO_ASE_SUPPORT
	if ( getenv("WITH_ASE") != NULL ) {
        fpga_result res = fpgaReadMMIO64(accel_handle, 0, offset, dest);
		fpga_assert(res);
	}
	else
#endif // !NO_ASE_SUPPORT
		*dest = MAPPED_MMIO(mmio_ptr, offset);
}

fpga_result OPAE_SIMPLE_WRAPPER_debug_read ( fpga_handle accel_handle, volatile uint64_t* mmio_ptr ) {
	fpga_result res = FPGA_OK;

    // Mapped MMIO access
	if ( getenv("WITH_ASE") != NULL ) {
        printf("Mapper MMIO read: AFU_DFH_REG     %016lx\n", MAPPED_MMIO(mmio_ptr, AFU_DFH_REG  ) );
        printf("Mapper MMIO read: AFU_ID_LO       %016lx\n", MAPPED_MMIO(mmio_ptr, AFU_ID_LO    ) );
        printf("Mapper MMIO read: AFU_ID_HI       %016lx\n", MAPPED_MMIO(mmio_ptr, AFU_ID_HI    ) );
        printf("Mapper MMIO read: AFU_NEXT        %016lx\n", MAPPED_MMIO(mmio_ptr, AFU_NEXT     ) );
        printf("Mapper MMIO read: AFU_RESERVED    %016lx\n", MAPPED_MMIO(mmio_ptr, AFU_RESERVED ) );
    }
	else {
		uint64_t data = 0;
		// DFL
		res = fpgaReadMMIO64(accel_handle, 0, AFU_DFH_REG, &data);
		fpga_assert(res);
		printf("AFU_DFH_REG = %016lx\n", data);
		res = fpgaReadMMIO64(accel_handle, 0, AFU_ID_LO, &data);
		fpga_assert(res);
		printf("AFU ID LO = %016lx\n", data);
		res = fpgaReadMMIO64(accel_handle, 0, AFU_ID_HI, &data);
		fpga_assert(res);
		printf("AFU ID HI = %016lx\n", data);
		res = fpgaReadMMIO64(accel_handle, 0, AFU_NEXT, &data);
		fpga_assert(res);
		printf("AFU NEXT = %016lx\n", data);
		res = fpgaReadMMIO64(accel_handle, 0, AFU_RESERVED, &data);
		fpga_assert(res);
		printf("AFU RESERVED = %016lx\n", data);
	}

	return res;
}

// fpga_result
// OPAE_SVC_WRAPPER::findAndOpenAccel(const char* accel_uuid){
// fpga_result OPAE_SIMPLE_WRAPPER_init ( fpga_handle* accel_handle, const char *accel_uuid ) {
//     fpga_result res = FPGA_OK;
	
// 	if ( accel_handle == NULL ) {									
// 		return FPGA_INVALID_PARAM;
// 	}
// 	// Compose the filter object
// 	fpga_properties filter = NULL;
//     res = fpgaGetProperties(NULL, &filter);
// 	ON_ERR_GOTO_local(res, out_exit, "creating properties object");
//     res = fpgaPropertiesSetObjectType(filter, FPGA_ACCELERATOR);
// 	ON_ERR_GOTO_local(res, out_destroy_prop, "setting object type");

//     // Add the desired UUID to the filter
//     fpga_guid guid;
// 	if (uuid_parse(accel_uuid, guid) < 0) {
// 		fprintf(stderr, "Error parsing guid '%s'\n", accel_uuid);
// 		goto out_exit;
// 	}	
// 	res = fpgaPropertiesSetGUID(filter, guid);
// 	ON_ERR_GOTO_local(res, out_destroy_prop, "setting GUID");

//     // How many accelerators match the requested properties?
//     uint32_t max_tokens;
//     res = fpgaEnumerate(&filter, 1, NULL, 0, &max_tokens);
// 	ON_ERR_GOTO_local(res, out_destroy_prop, "enumerating AFCs");

//     // Now that the number of matches is known, allocate a token vector large enough to hold them.
//     fpga_token* tokens = (fpga_token*)malloc(sizeof(fpga_token) * max_tokens);
//     if ( NULL == tokens ) {
//         res = fpgaDestroyProperties(&filter);
//         return FPGA_NO_MEMORY;
//     }

//     // Enumerate and get the tokens
//     uint32_t num_matches;
//     res = fpgaEnumerate(&filter, 1, tokens, max_tokens, &num_matches);
// 	ON_ERR_GOTO_local(res, out_destroy_prop, "enumerating AFCs");
//     fpga_token accel_token;
//     res = FPGA_NOT_FOUND;
//     for ( uint32_t i = 0; i < num_matches; i++ ) {
//         accel_token = tokens[i];
//         res = fpgaOpen(accel_token, accel_handle, 0);
// 		ON_ERR_GOTO_local(res, out_destroy_tok, "opening AFC");
//         // Success?
//         if (FPGA_OK == res) break;
//     }

// 	// Map MMOP address space
// 	res = fpgaMapMMIO(*accel_handle, 0, NULL);
// 	ON_ERR_GOTO_local(res, out_close, "mapping MMIO space");

// 	// Set up interrupts
// 	uint64_t data = 0;
// 	res = fpgaReadMMIO64(*accel_handle, 0, HLS_INT_ENABLE, &data);
// 	ON_ERR_GOTO_local(res, out_unmap_mmio, "reading from MMIO");
// #ifdef DEBUG_OSW
// 	printf("Interrupt enabled = %08lx\n", data);
// #endif

// 	int interruptEnabled = data & 0x1u;
// 	if ( !interruptEnabled ) {
// 		uint64_t enableWrite = data | 0x1u;
// 		res = fpgaWriteMMIO64(*accel_handle, 0, HLS_INT_ENABLE, enableWrite);
// 		ON_ERR_GOTO_local(res, out_unmap_mmio, "setting interrupt enable");
// 		res = fpgaReadMMIO64(*accel_handle, 0, HLS_INT_ENABLE, &data);
// 		ON_ERR_GOTO_local(res, out_unmap_mmio, "reading from MMIO");
// 	}

// #ifdef DEBUG_OSW
// 	res = OPAE_SIMPLE_WRAPPER_debug_read( *accel_handle );
// #endif // DEBUG_OSW

// 	// Reset AFU
// 	// res = fpgaReset( *accel_handle );
	
//     return res;
// }


volatile void * OPAE_SIMPLE_WRAPPER_allocate_io_buffer (
												fpga_handle accel_handle,
												ssize_t size,
												uint64_t *wsid,
												uint64_t *io_addr
											) {
    fpga_result res;
    volatile void* buf;

    int flags = 0;
    res = fpgaPrepareBuffer(accel_handle, size, (void**)&buf, wsid, flags);
    fpga_assert(res);

    // Get the physical address of the buffer for the accelerator
    res = fpgaGetIOAddress(accel_handle, *wsid, io_addr);
    fpga_assert(res);

#ifdef DEBUG_OSW
	printf("%s:%d io_addr %016lx:\n", __FILE__, __LINE__, *io_addr );
    printf("%s:%d buf %p:\n", __FILE__, __LINE__, buf );
#endif // DEBUG_OSW

    return buf;
}

fpga_result OPAE_SIMPLE_WRAPPER_call_afu (
							fpga_handle accel_handle, 
                            uint16_t	erasure_pattern,	 	
                            uint16_t	survived_cells,
                            uint32_t	cell_length,
							uint32_t 	sleep_time_us,
							fpga_event_handle *fpgaInterruptEvent,
							uint32_t	measure_latency,
							volatile uint64_t * mmio_ptr,
							FILE* 		fd_latency
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
	uint64_t cell_length_64			= cell_length / LINE_BYTE_WIDTH;
	rs_erasure_csrs |= erasure_pattern_64 	<< 0u ;
	rs_erasure_csrs |= survived_cells_64 	<< 16u;
	rs_erasure_csrs |= cell_length_64		<< 32u;

	mmio64_write ( accel_handle, mmio_ptr, KERNEL_ARG_RS_ERASURE_CSR_REG, rs_erasure_csrs );
#ifdef DEBUG_OSW
	printf("%s:%d write @%x, value = %lx\n", __FILE__, __LINE__, KERNEL_ARG_RS_ERASURE_CSR_REG, rs_erasure_csrs);
#endif // DEBUG_OSW

	////////////////////////////////
	// Create event for interrupt //
	////////////////////////////////
#ifdef INTERRUPT_EVENTS
	// Register user interrupt with event accel_handle
	res = fpgaCreateEventHandle(fpgaInterruptEvent);
	fpga_assert(res);
	uint32_t flags = 0; // uses IRQ bit 0, see instantiation of acmm_ccip_host_wr in afu.sv
	res = fpgaRegisterEvent(accel_handle, FPGA_EVENT_INTERRUPT, fpgaInterruptEvent, flags);
	fpga_assert(res);
#endif // INTERRUPT_EVENTS

	//////////////////////////////
	// Wait for AFU to be ready //
	//////////////////////////////
	// Poll on busy register
	// TODO: is there a cleaner way?
	uint64_t status_val;
	#define SLEEP_TIME_US 10000
	do {
		usleep( SLEEP_TIME_US );
		mmio64_read ( accel_handle, mmio_ptr, KERNEL_STATUS, &status_val );
	#ifdef DEBUG_OSW
		print_kernel_status(status_val);
	#endif // DEBUG_OSW
	} while ( status_val & KERNEL_REGISTER_MAP_BUSY_MASK );

	////////////////////////
	// Start and wait AFU //
	////////////////////////
	// Start measure by macro
	if ( measure_latency ) {
		MEASURE_LATENCY_START(start);
	}

	// fpga_result start_and_wait_afu(fpga_handle afc_handle, struct pollfd *pfd, int *poll_res)
#ifdef INTERRUPT_EVENTS
	pfd.events = POLLIN;
	res = fpgaGetOSObjectFromEventHandle(fpgaInterruptEvent, &pfd.fd);
	fpga_assert(res);
#endif // INTERRUPT_EVENTS

	// Start the AFU by writing a '1' into the start register
	mmio32_write(accel_handle, mmio_ptr, KERNEL_START, KERNEL_START_VALUE);
#ifdef DEBUG_OSW
	printf("%s:%d write @%x, value = %x\n", __FILE__, __LINE__, KERNEL_START, KERNEL_START_VALUE);
#endif // DEBUG_OSW

#ifdef INTERRUPT_EVENTS
	// Wait for interrupt with poll()
	poll_res = poll(&pfd, 1, POLL_TIMEOUT_MS);
#ifdef DEBUG_OSW
	printf("%s:%d poll_res = %d\n", __FILE__, __LINE__, poll_res);
	// Check poll errors
	if ( poll_res <= 0 ) {
		printf("Poll error errno = %s\n", strerror(errno));
	}
	else if ( poll_res == 0 ) {
		printf("Error: Poll timeout \n");
	}
	else {
		printf("Poll success. Return = %d\n", poll_res);
	}
#endif // DEBUG_OSW

#else // !INTERRUPT_EVENTS
	// Active polling on AFU
	do {
		usleep( SLEEP_TIME_US );
		mmio64_read ( accel_handle, mmio_ptr, KERNEL_STATUS, &status_val );
	#ifdef DEBUG_OSW
		print_kernel_status(status_val);
	#endif // DEBUG_OSW
	} while ( !(status_val & KERNEL_REGISTER_MAP_DONE_MASK) );
#endif // INTERRUPT_EVENTS

	// Clear interrupt
	// NOTE: this is necessary across calls regardless of INTERRUPT_EVENTS
	mmio64_read ( accel_handle, mmio_ptr, KERNEL_CLEAR_INTERRUPT, &status_val );
#ifdef DEBUG_OSW
	printf("%s:%d Read from KERNEL_CLEAR_INTERRUPT...\n", __FILE__, __LINE__);
	print_kernel_status(status_val);
#endif // DEBUG_OSW

	// End measure by macro
	if ( measure_latency ) {
		MEASURE_LATENCY_END_AND_PRINT(start, time_sec, fd_latency);
	}

	return res;
}


// fpga_result OPAE_SIMPLE_WRAPPER_cleanup( fpga_handle accel_handle, 
// 											fpga_event_handle *fpgaInterruptEvent, 
// 											uint64_t workspace_id_in,  
// 											uint64_t workspace_id_out 
// // 											){
// 	fpga_result res = FPGA_OK;

// 	if ( accel_handle == NULL ) {									
// 		return FPGA_INVALID_PARAM;
// 	}

// 	// Cleanup event accel_handle			
// 	if ( fpgaInterruptEvent != NULL ) {									
// 		res = fpgaUnregisterEvent(accel_handle, FPGA_EVENT_INTERRUPT, *fpgaInterruptEvent);
// 		res = fpgaDestroyEventHandle(fpgaInterruptEvent);
// 	}

// 	// Release I/O buffers
// 	res = fpgaReleaseBuffer(accel_handle, workspace_id_out);
// 	res = fpgaReleaseBuffer(accel_handle, workspace_id_in);

// 	// Unmap MMIO space 
// 	res = fpgaUnmapMMIO(accel_handle, 0);

// 	// Release accelerator 
// 	res = fpgaClose(accel_handle);

// 	return res;
// }