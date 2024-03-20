// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT

#include <stdint.h>
#include <stdio.h> // for printf()
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>     // for assert()
#include <uuid/uuid.h>  // for uuid_parse
#include <poll.h>       // for poll()
#include <errno.h>

#include <opae/fpga.h>

// State from the AFU's JSON file, extracted using OPAE's afu_json_mgr script
#include "afu_json_info.h"

// Register map emitted for DFL
#include "afu_regmap.h"

// Utility functions
#include "sycl_afu_utils.h"

// Macros
#define CACHELINE_BYTES         64                              // Number of bytes of a cache line
#define CL_ALIGN(phy_addr)      (phy_addr / CACHELINE_BYTES)    // Cache-line aligned physical address
#define SIZE_BUFFERS(n)         (CACHELINE_BYTES * (n))         // Size of I/O buffers in cache lines

#define INTERRUPT_EVENTS

int main(int argc, char *argv[]) {
    static const uint32_t max_handles = 32;
    fpga_handle accel_handles[max_handles];
    uint32_t num_handles = max_handles;
    bool is_ase_sim = false;

    // MMIO pointers and metadata
    volatile uint8_t * device_read;
	volatile uint8_t * device_write;
    uint64_t wsid_in, wsid_out;
    uint64_t buf_pa_in, buf_pa_out;
    uint64_t cell_length_byte = 128;

    // FPGA error code
    volatile fpga_result res = FPGA_OK;
    fpga_event_handle fpgaInterruptEvent;
    uint64_t length_lines = 1;

    if ( argc > 1 ) {
        length_lines = atoi(argv[1]);
    }

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
        device_read  = (volatile uint8_t*)alloc_buffer(accel_handles[i], SIZE_BUFFERS(length_lines), &wsid_in , &buf_pa_in );
        device_write = (volatile uint8_t*)alloc_buffer(accel_handles[i], SIZE_BUFFERS(length_lines), &wsid_out, &buf_pa_out);

        // Check addresses are 41 bits
        #define BIT_MASK_41 ((uint64_t)0x01fffffffffful)
        assert ( (buf_pa_in  & (~BIT_MASK_41)) == (uint64_t)0 );
        assert ( (buf_pa_out & (~BIT_MASK_41)) == (uint64_t)0 );

        assert(NULL != device_read);
        assert(NULL != device_write);

        // Init input buffer
        for ( unsigned int j = 0; j < SIZE_BUFFERS(length_lines)/sizeof(uint32_t); j++ ) {
            // "SYCL" = 0x4c435953
            ((uint32_t*)device_read)[j] = (uint32_t)0x4c435953u;
        }   
        device_read[SIZE_BUFFERS(length_lines)-1] = '\0';

        // Init output buffer
        for ( unsigned int j = 0; j < SIZE_BUFFERS(length_lines)/sizeof(uint32_t); j++ ) {
            ((uint32_t*)device_write)[j] = (uint32_t)0xdeadbeefu;
        }   

        // Print-out buffers content
        printf("%s:%d: device_read:\n", __FILE__, __LINE__);
        for ( unsigned int j = 0; j < SIZE_BUFFERS(length_lines); j++ ) {
            printf("%hhx ", ((uint8_t*)device_read)[j]);
        }   
        printf("\n");
        printf("%s:%d: device_write:\n", __FILE__, __LINE__);
        for ( unsigned int j = 0; j < SIZE_BUFFERS(length_lines); j++ ) {
            printf("%hhx ", ((uint8_t*)device_write)[j]);
        }   
        printf("\n");

        printf("%s:%d Write argument CSRs...\n", __FILE__, __LINE__);
        // Write physical addresses to AFU CSR
        res = fpgaWriteMMIO64(accel_handles[i], 0, KERNEL_ARG_DEVICE_READ_REG, buf_pa_in);
        assert(FPGA_OK == res);
        printf("%s:%d write @%08x, value = %016lx\n", __FILE__, __LINE__, KERNEL_ARG_DEVICE_READ_REG, buf_pa_in);
        res = fpgaWriteMMIO64(accel_handles[i], 0, KERNEL_ARG_DEVICE_WRITE_REG, buf_pa_out);
        assert(FPGA_OK == res);
        printf("%s:%d write @%08x, value = %016lx\n", __FILE__, __LINE__, KERNEL_ARG_DEVICE_WRITE_REG, buf_pa_out);

        // Write number of lines to loopback
        res = fpgaWriteMMIO64(accel_handles[i], 0, KERNEL_ARG_RS_LENGTH_LINES_REG, length_lines);
        printf("%s:%d write @%08x, value = %016lx\n", __FILE__, __LINE__, KERNEL_ARG_RS_LENGTH_LINES_REG, length_lines);

    #ifdef INTERRUPT_EVENTS
        ////////////////////////////////
        // Create event for interrupt //
        ////////////////////////////////

        res = fpgaCreateEventHandle(&fpgaInterruptEvent);
        if ( res != FPGA_OK ) {
            printf("%s:%d res = %s\n", __FILE__, __LINE__, fpgaErrStr(res));
        }
        // Register user interrupt with event accel_handle
        uint32_t flags = 0; // uses IRQ bit 0, see instantiation of acmm_ccip_host_wr in afu.sv
        res = fpgaRegisterEvent(accel_handles[i], FPGA_EVENT_INTERRUPT, fpgaInterruptEvent, flags);
        assert(FPGA_OK == res);

        #define POLL_TIMEOUT_MS 1000
        // Pass the poll file descriptor
        struct pollfd pfd;
        pfd.events = POLLIN;
        res = fpgaGetOSObjectFromEventHandle(fpgaInterruptEvent, &pfd.fd);
        assert(FPGA_OK == res);

    #endif // INTERRUPT_EVENTS

        //////////////////////////////
        // Wait for AFU to be ready //
        //////////////////////////////
        // Poll on status register
        // TODO: is there a cleaner way?
        uint64_t status_val;
        #define SLEEP_TIME_US 1000000
        printf("%s:%d Wait for !BUSY...\n", __FILE__, __LINE__);
        do {
            usleep( SLEEP_TIME_US );
            res = fpgaReadMMIO64(accel_handles[i], 0, KERNEL_STATUS, &status_val);
            assert(FPGA_OK == res);
            print_kernel_status(status_val);
        } while ( status_val & KERNEL_REGISTER_MAP_BUSY_MASK );

        ////////////////////////
        // Start and wait AFU //
        ////////////////////////
        // Writing a '1' into the start register
        printf("%s:%d Write to START...\n", __FILE__, __LINE__);
        res = fpgaWriteMMIO32(accel_handles[i], 0, KERNEL_START, KERNEL_START_VALUE);
        assert(FPGA_OK == res);

    #ifdef INTERRUPT_EVENTS
        printf("%s:%d Calling poll()...\n", __FILE__, __LINE__);
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
    #else // !INTERRUPT_EVENTS
        // Active polling on AFU
        printf("%s:%d Wait for DONE...\n", __FILE__, __LINE__);
        do {
            usleep( SLEEP_TIME_US );
            // Compiler fence
            res = fpgaReadMMIO64(accel_handles[i], 0, KERNEL_STATUS, &status_val);
            assert(FPGA_OK == res);
            print_kernel_status(status_val);
        } while ( !(status_val & KERNEL_REGISTER_MAP_DONE_MASK) );
    #endif // !INTERRUPT_EVENTS

        // Print-out buffers content
        printf("%s:%d: device_read:\n", __FILE__, __LINE__);
        for ( unsigned int j = 0; j < SIZE_BUFFERS(length_lines); j++ ) {
            printf("%hhx ", ((uint8_t*)device_read)[j]);
        }   
        printf("\n");
        printf("%s:%d: device_write:\n", __FILE__, __LINE__);
        for ( unsigned int j = 0; j < SIZE_BUFFERS(length_lines); j++ ) {
            printf("%hhx ", ((uint8_t*)device_write)[j]);
        }   
        printf("\n");

        // Print the string written by the FPGA
        printf("%s:%d: FPGA says: %s\n", __FILE__, __LINE__, (uint8_t*)device_write);

        // Clean-up
    #ifdef INTERRUPT_EVENTS
        // Cleanup event accel_handle			
        if ( fpgaInterruptEvent != NULL ) {			
            res = fpgaUnregisterEvent(accel_handles[i], FPGA_EVENT_INTERRUPT, fpgaInterruptEvent);
            assert(FPGA_OK == res);
            res = fpgaDestroyEventHandle(&fpgaInterruptEvent);
            assert(FPGA_OK == res);
        }
    #endif // !INTERRUPT_EVENTS

        // Release I/O buffers
        res = fpgaReleaseBuffer(accel_handles[i], wsid_in);
        assert(FPGA_OK == res);
        res = fpgaReleaseBuffer(accel_handles[i], wsid_out);
        assert(FPGA_OK == res);
        // Unmap MMIO space
        if ( ! is_ase_sim ) {
            res = fpgaUnmapMMIO(accel_handles[i], 0);
            assert(FPGA_OK == res);
        }
        res = fpgaClose(accel_handles[i]);
        assert(FPGA_OK == res);

    } // i, num_handles

    return 0;

out_exit:
	return res;
}
