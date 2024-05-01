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
#include "afu_regmap.h"

// Utility functions
#include "sycl_afu_utils.h"

// RS header
#include "rs_erasure.hpp"

#define INTERRUPT_EVENTS
#define NUM_ERASURES RS_P

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
    volatile uint64_t * mmio_ptr [max_handles];
    res = connect_to_matching_accels(AFU_ACCEL_UUID, &num_handles, accel_handles,
                                   &is_ase_sim, (volatile uint64_t**)&mmio_ptr);
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
        reconstructed_blocks_out = (volatile uint8_t*)alloc_buffer(accel_handles[i], RS_OUTPUT_SIZE(cell_length_byte, NUM_ERASURES), &wsid_out, &buf_pa_out);

        assert(NULL != rs_erasure_input);
        assert(NULL != reconstructed_blocks_out);

        if ( accel_handles[i] == NULL ) {
            return FPGA_INVALID_PARAM;
        }

        /////////////////////////
        // Load AFU parameters //
        /////////////////////////
        // Write physical address to AFU CSR
        // res = fpgaWriteMMIO64(accel_handles[i], 0, KERNEL_ARG_DEVICE_READ_REG, buf_pa_in);
        // fpga_assert(res);
        MAPPED_MMIO(mmio_ptr[i], KERNEL_ARG_DEVICE_READ_REG) = buf_pa_in;
        printf("%s:%d write @%x, value = %lx\n", __FILE__, __LINE__, KERNEL_ARG_DEVICE_READ_REG, buf_pa_in);

        // Write physical address to AFU CSR
        // res = fpgaWriteMMIO64(accel_handles[i], 0, KERNEL_ARG_DEVICE_WRITE_REG, buf_pa_out);
        // fpga_assert(res);
        MAPPED_MMIO(mmio_ptr[i], KERNEL_ARG_DEVICE_WRITE_REG) = buf_pa_out;
        printf("%s:%d write @%x, value = %ld\n", __FILE__, __LINE__, KERNEL_ARG_DEVICE_WRITE_REG, buf_pa_out);

        // Serialize CSR inputs
        uint64_t rs_erasure_csrs = 0;
        uint64_t erasure_pattern_64 	= 0b11000;
        uint64_t survived_cells_64 		= 0b00111;
        uint64_t cell_length_64			= cell_length_byte / LINE_BYTE_WIDTH;
        rs_erasure_csrs |= erasure_pattern_64 	<< 0u ;
        rs_erasure_csrs |= survived_cells_64 	<< 16u;
        rs_erasure_csrs |= cell_length_64		<< 32u;

        // res = fpgaWriteMMIO64(accel_handles[i], 0, KERNEL_ARG_RS_ERASURE_CSR_REG, rs_erasure_csrs);
        // fpga_assert(res);
        MAPPED_MMIO(mmio_ptr[i], KERNEL_ARG_RS_ERASURE_CSR_REG) = rs_erasure_csrs;
        printf("%s:%d write @%x, value = %lx\n", __FILE__, __LINE__, KERNEL_ARG_RS_ERASURE_CSR_REG, rs_erasure_csrs);

        // Seed the PRNG
        unsigned int prng_seed = 54656;
        srand(prng_seed);
        printf("%s:%d: rs_erasure_input\n", __FILE__, __LINE__);
        for ( unsigned int j = 0; j < (cell_length_byte * RS_K); j++ ){
            rs_erasure_input[j] = rand();
            printf("%02x ", rs_erasure_input[j]);
			if ( ((j+1) % LINE_BYTE_WIDTH) == 0 ) {
				printf("\n");
			}
        }
        printf("\n");

        ////////////////////////////////
        // Create event for interrupt //
        ////////////////////////////////
    #ifdef INTERRUPT_EVENTS
        // Register user interrupt with event accel_handle
        res = fpgaCreateEventHandle(&fpgaInterruptEvent);
        fpga_assert(res);
        uint32_t flags = 0; // uses IRQ bit 0, see instantiation of acmm_ccip_host_wr in afu.sv
        res = fpgaRegisterEvent(accel_handles[i], FPGA_EVENT_INTERRUPT, fpgaInterruptEvent, flags);
        fpga_assert(res);

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
            // res = fpgaReadMMIO64(accel_handles[i], 0, KERNEL_STATUS, &status_val);
            // fpga_assert(res);
            status_val = MAPPED_MMIO(mmio_ptr[i], KERNEL_STATUS);
            print_kernel_status(status_val);
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
        fpga_assert(res);
    #endif // INTERRUPT_EVENTS

        // Start the AFU by writing a '1' into the start register
        // res = fpgaWriteMMIO64(accel_handles[i], 0, KERNEL_START, KERNEL_START_VALUE);
        // fpga_assert(res);
        MAPPED_MMIO(mmio_ptr[i], KERNEL_START) = KERNEL_START_VALUE;
        printf("%s:%d write @%x, value = %x\n", __FILE__, __LINE__, KERNEL_START, KERNEL_START_VALUE);

    #ifndef INTERRUPT_EVENTS
        // Active polling on AFU
        do {
            usleep( SLEEP_TIME_US );
            // res = fpgaReadMMIO64(accel_handles[i], 0, KERNEL_STATUS, &status_val);
            // fpga_assert(res);
            status_val = MAPPED_MMIO(mmio_ptr[i], KERNEL_STATUS);
            print_kernel_status(status_val);
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

        // Clear interrupt
        printf("%s:%d Read from KERNEL_CLEAR_INTERRUPT...\n", __FILE__, __LINE__);
        // res = fpgaReadMMIO64(accel_handles[0], 0, KERNEL_CLEAR_INTERRUPT, &status_val);
        // fpga_assert(res);
        status_val = MAPPED_MMIO(mmio_ptr[i], KERNEL_CLEAR_INTERRUPT);
        print_kernel_status(status_val);

    #endif // INTERRUPT_EVENTS

        // Check output buffer
		printf("%s:%d: reconstructed_blocks_out\n", __FILE__, __LINE__);
        for ( unsigned int j = 0; j < RS_OUTPUT_SIZE(cell_length_byte, NUM_ERASURES); j++ ){
            printf("%02x ", reconstructed_blocks_out[j]);
			if ( ((j+1) % LINE_BYTE_WIDTH) == 0 ) {
				printf("\n");
			}
        }
        printf("\n");

        //////////////
        // Clean up //
        //////////////

    #ifdef INTERRUPT_EVENTS
        // Cleanup event accel_handle
        if ( fpgaInterruptEvent != NULL ) {
            res = fpgaUnregisterEvent(accel_handles[i], FPGA_EVENT_INTERRUPT, fpgaInterruptEvent);
            // res = fpgaDestroyEventHandle(fpgaInterruptEvent);
        }
    #endif // INTERRUPT_EVENTS

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
