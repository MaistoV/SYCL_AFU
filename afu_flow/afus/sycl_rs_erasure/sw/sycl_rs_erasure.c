// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <uuid/uuid.h>
#include <poll.h> // For poll()
#include <errno.h>

#include <opae/fpga.h>

// State from the AFU's JSON file, extracted using OPAE's afu_json_mgr script
#include "afu_json_info.h"

// Register map emitted for DFL
#include "afu_dfl_regmap.h"
// Register map emitted by SYCL
#include "register_map_offsets.hpp"
// RS header
#include "rs_erasure.hpp"

/*
 * macro to check return codes, print error message, and goto cleanup label
 * NOTE: this changes the program flow (uses goto)!
 */
int s_error_count = 0;
void print_err(const char *s, fpga_result res) {
	fprintf(stderr, "%s:%d: Error %s: %s\n", __FILE__, __LINE__, s, fpgaErrStr(res));
}
#define ON_ERR_GOTO(res, label, desc) \
	do                                \
	{                                 \
		if ((res) != FPGA_OK)         \
		{                             \
			print_err((desc), (res)); \
			s_error_count += 1;       \
			goto label;               \
		}                             \
	} while (0)

//
// Search for all accelerators matching the requested properties and
// connect to them. The input value of *num_handles is the maximum
// number of connections allowed. (The size of accel_handles.) The
// output value of *num_handles is the actual number of connections.
//
static fpga_result
connect_to_matching_accels(const char *accel_uuid,
                           uint32_t *num_handles,
                           fpga_handle *accel_handles,
                           bool *is_ase_sim) {
    fpga_properties filter = NULL;
    fpga_guid guid;
    const uint32_t max_tokens = 32;
    fpga_token accel_tokens[max_tokens];
    uint32_t num_matches;
    fpga_result res;

    assert(num_handles && *num_handles);
    assert(accel_handles);

    // Limit num_handles to max_tokens. We could be smarter and dynamically
    // allocate accel_tokens.
    if (*num_handles > max_tokens)
        *num_handles = max_tokens;

    // Don't print verbose messages in ASE by default
    setenv("ASE_LOG", "0", 0);
    *is_ase_sim = false;

    // Set up a filter that will search for an accelerator
    fpgaGetProperties(NULL, &filter);
    fpgaPropertiesSetObjectType(filter, FPGA_ACCELERATOR);

    // Add the desired UUID to the filter
    uuid_parse(accel_uuid, guid);
    fpgaPropertiesSetGUID(filter, guid);

    // Do the search across the available FPGA contexts
    res = fpgaEnumerate(&filter, 1, accel_tokens, *num_handles, &num_matches);
    if (*num_handles > num_matches)
        *num_handles = num_matches;

    if ((FPGA_OK != res) || (num_matches < 1))
    {
        fprintf(stderr, "Accelerator %s not found!\n", accel_uuid);
        goto out_destroy;
    }

    // Open accelerators
    uint32_t num_found = 0;
    for (uint32_t i = 0; i < *num_handles; i += 1) {
        res = fpgaOpen(accel_tokens[i], &accel_handles[num_found], 0);
        if (FPGA_OK == res) {
            num_found += 1;

            // While the token is available, check whether it is for HW
            // or for ASE simulation, recording it so probeForASE() below
            // doesn't have to run through the device list again.
            fpga_properties accel_props;
            uint16_t vendor_id, dev_id;
            fpgaGetProperties(accel_tokens[i], &accel_props);
            fpgaPropertiesGetVendorID(accel_props, &vendor_id);
            fpgaPropertiesGetDeviceID(accel_props, &dev_id);
            *is_ase_sim = (vendor_id == 0x8086) && (dev_id == 0xa5e);
        }

        fpgaDestroyToken(&accel_tokens[i]);

        // Map MMIO address space
        res = fpgaMapMMIO(accel_handles[i], 0, NULL);
        if (FPGA_OK != res) {
            return res;
        }

        // Not supported by vfio plugin
        // Reset AFC 
        // res = fpgaReset( accel_handles[i] );
        // if (FPGA_OK != res) {
        //     return res;
        // }

    }
    *num_handles = num_found;
    if (0 != num_found) res = FPGA_OK;

  out_destroy:
    fpgaDestroyProperties(&filter);

    return res;
}


//
// Allocate a buffer in I/O memory, shared with the FPGA.
//
static volatile void* alloc_buffer(fpga_handle accel_handle,
                                   ssize_t size,
                                   uint64_t *wsid,
                                   uint64_t *io_addr) {
    fpga_result res;
    volatile void* buf;

    res = fpgaPrepareBuffer(accel_handle, size, (void*)&buf, wsid, 0);
    if (FPGA_OK != res) return NULL;

    // Get the physical address of the buffer in the accelerator
    res = fpgaGetIOAddress(accel_handle, *wsid, io_addr);
    assert(FPGA_OK == res);

    return buf;
}


#define SIZE 64 // Temp

int main(int argc, char *argv[]) {
    static const uint32_t max_handles = 32;
    fpga_handle accel_handles[max_handles];
    uint32_t num_handles = max_handles;
    bool is_ase_sim = false;

    // MMIO pointers and metadata
    volatile uint8_t * rs_erasure_input        ;
	volatile uint8_t * reconstructed_blocks_out;
    uint64_t wsid_in, wsid_out;
    uint64_t buf_pa_in, buf_pa_out;
    uint64_t cell_length_byte = 128;

    // FPGA error code
    volatile fpga_result res = FPGA_OK;
    fpga_event_handle fpgaInterruptEvent;

    // Find and connect to the accelerators
    res = connect_to_matching_accels(AFU_ACCEL_UUID, &num_handles, accel_handles,
                                   &is_ase_sim);
    if ( (res != FPGA_OK) || (0 == num_handles) ) {
        exit(1);
    }
    if ( is_ase_sim ) {
        printf("   *** ASE only detects a single AFU (port 0) ***\n");
    }

    printf("Found %d instance(s) of AFU:\n\n", num_handles);

    for ( uint32_t i = 0; i < num_handles; i += 1 ) { // i, num_handles
        printf("Handle %d\n", i);

        // Allocate MMIO buffers
        rs_erasure_input         = (volatile uint8_t*)alloc_buffer(accel_handles[i], RS_INPUT_SIZE (cell_length_byte), &wsid_in , &buf_pa_in );
        reconstructed_blocks_out = (volatile uint8_t*)alloc_buffer(accel_handles[i], RS_OUTPUT_SIZE(cell_length_byte), &wsid_out, &buf_pa_out);
        // rs_erasure_input         = (volatile uint8_t*)alloc_buffer(accel_handles[i], getpagesize(), &wsid_in , &buf_pa_in );
        // reconstructed_blocks_out = (volatile uint8_t*)alloc_buffer(accel_handles[i], getpagesize(), &wsid_out, &buf_pa_out);

        assert(NULL != rs_erasure_input);
        assert(NULL != reconstructed_blocks_out);

        if ( accel_handles[i] == NULL ) {
            return FPGA_INVALID_PARAM;
        }
        
        ///////////////////////////////
        // Debug reads from DFL CSRs //
        ///////////////////////////////
        uint64_t data = 0;
        
        // DFL
        res = fpgaReadMMIO64(accel_handles[i], 0, AFU_DFH_REG, &data);
        ON_ERR_GOTO(res, out_exit, "Reading MMIO AFU_DFH_REG");

        res = fpgaReadMMIO64(accel_handles[i], 0, AFU_ID_LO, &data);
        ON_ERR_GOTO(res, out_exit, "Reading MMIO AFU_ID_LO");
        printf("AFU ID LO = %08lx\n", data);

        res = fpgaReadMMIO64(accel_handles[i], 0, AFU_ID_HI, &data);
        ON_ERR_GOTO(res, out_exit, "Reading MMIO AFU_ID_HI");
        printf("AFU ID HI = %08lx\n", data);

        res = fpgaReadMMIO64(accel_handles[i], 0, AFU_NEXT, &data);
        ON_ERR_GOTO(res, out_exit, "Reading MMIO AFU_NEXT");
        printf("AFU NEXT = %08lx\n", data);

        res = fpgaReadMMIO64(accel_handles[i], 0, AFU_RESERVED, &data);
        ON_ERR_GOTO(res, out_exit, "Reading MMIO AFU_RESERVED");
        printf("AFU RESERVED = %08lx\n", data);

        // SYCL Kernel
        res = fpgaReadMMIO64(accel_handles[i], 0, KERNEL_STATUS, &data);
        ON_ERR_GOTO(res, out_exit, "Reading MMIO KERNEL_STATUS");
        printf("AFU KERNEL_STATUS = %08lx\n", data);

        res = fpgaReadMMIO64(accel_handles[i], 0, KERNEL_START, &data);
        ON_ERR_GOTO(res, out_exit, "Reading MMIO KERNEL_START");
        printf("AFU KERNEL_START = %08lx\n", data);

        res = fpgaReadMMIO64(accel_handles[i], 0, KERNEL_START, &data);
        ON_ERR_GOTO(res, out_exit, "Reading MMIO KERNEL_START");
        printf("AFU KERNEL_START = %08lx\n", data);

        /////////////////////////
        // Load AFU parameters //
        /////////////////////////
        // Write physical address to AFU CSR
        res = fpgaWriteMMIO64(accel_handles[i], 0, KERNEL_ARG_DEVICE_READ_REG, buf_pa_in);
        printf("%s:%d write @%x, value = %lx\n", __FILE__, __LINE__, KERNEL_ARG_DEVICE_READ_REG, buf_pa_in);
        // Write virtual address to AFU CSR
        res = fpgaWriteMMIO64(accel_handles[i], 0, KERNEL_ARG_DEVICE_READ_REG, (uint64_t)rs_erasure_input);
        printf("%s:%d write @%x, value = %lx\n", __FILE__, __LINE__, KERNEL_ARG_DEVICE_READ_REG, rs_erasure_input);
        if ( res != FPGA_OK ) {
            printf("%s:%d res = %s\n", __FILE__, __LINE__, fpgaErrStr(res));
        }

        // Write physical address to AFU CSR
        res = fpgaWriteMMIO64(accel_handles[i], 0, KERNEL_ARG_DEVICE_WRITE_REG, buf_pa_out);
        printf("%s:%d write @%x, value = %lx\n", __FILE__, __LINE__, KERNEL_ARG_DEVICE_WRITE_REG, buf_pa_out);
        // Write virtual address to AFU CSR
        res = fpgaWriteMMIO64(accel_handles[i], 0, KERNEL_ARG_DEVICE_WRITE_REG, (uint64_t)reconstructed_blocks_out);
        printf("%s:%d write @%x, value = %lx\n", __FILE__, __LINE__, KERNEL_ARG_DEVICE_WRITE_REG, reconstructed_blocks_out);
        if ( res != FPGA_OK ) {
            printf("%s:%d res = %s\n", __FILE__, __LINE__, fpgaErrStr(res));
        }

        // Serialize CSR inputs
        uint64_t rs_erasure_csrs = 0;
        uint64_t erasure_pattern_64 	= 0b01000;
        uint64_t survived_cells_64 		= 0b00111;
        uint64_t cell_length_64			= cell_length_byte / CELL_BYTE_WIDTH;
        rs_erasure_csrs |= erasure_pattern_64 	<< 0u ;
        rs_erasure_csrs |= survived_cells_64 	<< 16u;
        rs_erasure_csrs |= cell_length_64		<< 32u;

        res = fpgaWriteMMIO64(accel_handles[i], 0, KERNEL_ARG_RS_ERASURE_CSR_REG, rs_erasure_csrs);
        printf("%s:%d write @%x, value = %lx\n", __FILE__, __LINE__, KERNEL_ARG_RS_ERASURE_CSR_REG, rs_erasure_csrs);
        if ( res != FPGA_OK ) {
            printf("%s:%d res = %s\n", __FILE__, __LINE__, fpgaErrStr(res));
        }

		printf("%s:%d: rs_erasure_input\n", __FILE__, __LINE__);
        for ( unsigned int j = 0; j < (SIZE * RS_K); j++ ){
            rs_erasure_input[j] = rand();
            printf("%02x ", rs_erasure_input[j]);
        }
        printf("\n");
        
        ////////////////////////////////
        // Create event for interrupt //
        ////////////////////////////////
    #ifdef INTERRUPT_EVENTS
        res = fpgaCreateEventHandle(&fpgaInterruptEvent);
        if ( res != FPGA_OK ) {
            printf("%s:%d res = %s\n", __FILE__, __LINE__, fpgaErrStr(res));
        }
        // Register user interrupt with event accel_handle
        uint32_t flags = 0; // uses IRQ bit 0, see instantiation of acmm_ccip_host_wr in afu.sv
        res = fpgaRegisterEvent(accel_handles[i], FPGA_EVENT_INTERRUPT, fpgaInterruptEvent, flags);
        if ( res != FPGA_OK ) {
            printf("%s:%d res = %s\n", __FILE__, __LINE__, fpgaErrStr(res));
        }
    #endif // INTERRUPT_EVENTS

        ////////////////////////////////
        // Wait for AFU to be ready
        ////////////////////////////////
        // Poll on busy register
        // TODO: is there a cleaner way?
        uint64_t status_val;
        #define SLEEP_TIME_US 10000
        do {
            usleep( SLEEP_TIME_US );
            res = fpgaReadMMIO64(accel_handles[i], 0, KERNEL_STATUS, &status_val);
            if ( res != FPGA_OK ) {
                printf("%s:%d res = %s\n", __FILE__, __LINE__, fpgaErrStr(res));
            }
            printf("%s:%d status_val = %0lx\n", __FILE__, __LINE__, status_val);
        } while ( status_val & KERNEL_REGISTER_MAP_BUSY_MASK );

        ////////////////////////
        // Start and wait AFU //
        ////////////////////////
        // fpga_result start_and_wait_afu(fpga_handle afc_handle, struct pollfd *pfd, int *poll_res)
    #ifdef INTERRUPT_EVENTS
        #define POLL_TIMEOUT_MS 1000
        struct pollfd pfd;
        pfd.events = POLLIN;
        res = fpgaGetOSObjectFromEventHandle(fpgaInterruptEvent, &pfd.fd);
        if ( res != FPGA_OK ) {
            printf("%s:%d res = %s\n", __FILE__, __LINE__, fpgaErrStr(res));
        }
    #endif // INTERRUPT_EVENTS

        // Start the AFU by writing a '1' into the start register
        res = fpgaWriteMMIO64(accel_handles[i], 0, KERNEL_START, KERNEL_START_VALUE);
        if ( res != FPGA_OK ) {
            printf("%s:%d res = %s\n", __FILE__, __LINE__, fpgaErrStr(res));
            return res;
        }

    #ifndef INTERRUPT_EVENTS
        // Active polling on AFU
        do {
            usleep( SLEEP_TIME_US );
            // Compiler fence
            res = fpgaReadMMIO64(accel_handles[i], 0, KERNEL_STATUS, &status_val);
            if ( res != FPGA_OK ) {
                printf("%s:%d res = %s\n", __FILE__, __LINE__, fpgaErrStr(res));
            }
            printf("%s:%d status_val = %0lx\n", __FILE__, __LINE__, status_val);
        } while ( !(status_val & KERNEL_REGISTER_MAP_DONE_MASK) );
    #endif // !INTERRUPT_EVENTS

    #ifdef INTERRUPT_EVENTS
        // Wait for interrupt with poll()
        int poll_res = poll(&pfd, 1, POLL_TIMEOUT_MS);
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
    #endif // INTERRUPT_EVENTS

        // Check output buffer
		printf("%s:%d: reconstructed_blocks_out\n", __FILE__, __LINE__);
        for ( unsigned int j = 0; j < SIZE; j++ ){
            printf("%02x ", reconstructed_blocks_out[j]);
        }
        printf("\n");

        //////////////
        // Clean up //
        //////////////

    #ifdef INTERRUPT_EVENTS
        // Cleanup event accel_handle			
        if ( fpgaInterruptEvent != NULL ) {									
            res = fpgaUnregisterEvent(accel_handles[i], FPGA_EVENT_INTERRUPT, fpgaInterruptEvent);
            res = fpgaDestroyEventHandle(fpgaInterruptEvent);
        }
    #endif // !INTERRUPT_EVENTS

        // Release I/O buffers
        fpgaReleaseBuffer(accel_handles[i], wsid_in);
        fpgaReleaseBuffer(accel_handles[i], wsid_out);
        // Unmap MMIO space
        fpgaUnmapMMIO(accel_handles[i], 0);
        fpgaClose(accel_handles[i]);

    } // i, num_handles

    return 0;

out_exit:
	return res;
}
