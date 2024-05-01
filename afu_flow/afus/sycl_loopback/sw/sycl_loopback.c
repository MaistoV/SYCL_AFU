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
    uint64_t status_val;

    if ( argc > 1 ) {
        length_lines = atoi(argv[1]);
    }

    // Find and connect to the accelerators
    uint64_t* mmio_ptr;
    res = connect_to_matching_accels(AFU_ACCEL_UUID, &num_handles, accel_handles,
                                   &is_ase_sim, &mmio_ptr);
    if ( (res != FPGA_OK) || (0 == num_handles) ) {
        exit(1);
    }
    if ( is_ase_sim ) {
        printf("   *** ASE only detects a single AFU (port 0) ***\n");
    #ifdef INTERRUPT_EVENTS
        fprintf(stderr, "[WARNING] ASE will error out in case of AVMM interrupts because of encoding, i.e. width=0");
    #endif // INTERRUPT_EVENTS
    }
    else {
        // assert(mmio_ptr);
    }

    printf("Found %d instance(s) of AFU:\n", num_handles);
    printf("Using handle 0\n\n");

    // Access mapped MMIO space
    if ( !is_ase_sim ) {
        // printf("%s:%d read AFU_DFH_REG      @%016lx: \n", __FILE__, __LINE__, mmio_ptr[AFU_DFH_REG]);
        // printf("%s:%d read AFU_ID_LO        @%016lx: \n", __FILE__, __LINE__, mmio_ptr[AFU_ID_LO]);
        // printf("%s:%d read AFU_ID_HI        @%016lx: \n", __FILE__, __LINE__, mmio_ptr[AFU_ID_HI]);
        // printf("%s:%d read AFU_NEXT         @%016lx: \n", __FILE__, __LINE__, mmio_ptr[AFU_NEXT]);
        // printf("%s:%d read AFU_RESET        @%016lx: \n", __FILE__, __LINE__, mmio_ptr[AFU_RESET]);
        // printf("%s:%d read AFU_IRQ_EN       @%016lx: \n", __FILE__, __LINE__, mmio_ptr[AFU_IRQ_EN]);
        // printf("%s:%d read KERNEL_STATUS    @%016lx: \n", __FILE__, __LINE__, mmio_ptr[KERNEL_STATUS]);
        // // printf("%s:%d read KERNEL_START @%016lx: \n", __FILE__, __LINE__, mmio_ptr[AFU_RESET]);
        // // Read-any
        // printf("%s:%d read KERNEL_ARG_DEVICE_READ_REG     @%016lx: \n", __FILE__, __LINE__, mmio_ptr[KERNEL_ARG_DEVICE_READ_REG]);
        // printf("%s:%d read KERNEL_ARG_DEVICE_WRITE_REG    @%016lx: \n", __FILE__, __LINE__, mmio_ptr[KERNEL_ARG_DEVICE_WRITE_REG]);
        // printf("%s:%d read KERNEL_ARG_RS_LENGTH_LINES_REG @%016lx: \n", __FILE__, __LINE__, mmio_ptr[KERNEL_ARG_RS_LENGTH_LINES_REG]);
    }
    else {
        // Disable interrupts in simluation
        printf("%s:%d Disable AFU interrupts via CSR write...\n", __FILE__, __LINE__);
        res = fpgaWriteMMIO64(accel_handles[0], 0, AFU_IRQ_EN, ~(AFU_IRQ_EN_VALUE));
        fpga_assert(res);
        printf("%s:%d write @%08x, value = %016lx\n", __FILE__, __LINE__, AFU_IRQ_EN, ~(AFU_IRQ_EN_VALUE) & AFU_IRQ_EN_MASK);
    }

    // Allocate MMIO buffers
    device_read  = (volatile uint8_t*)alloc_buffer(accel_handles[0], SIZE_BUFFERS(length_lines), &wsid_in , &buf_pa_in );
    device_write = (volatile uint8_t*)alloc_buffer(accel_handles[0], SIZE_BUFFERS(length_lines), &wsid_out, &buf_pa_out);

    printf("%s:%d: buf_pa_in : %016lx\n", __FILE__, __LINE__, buf_pa_in );
    printf("%s:%d: buf_pa_out: %016lx\n", __FILE__, __LINE__, buf_pa_out);

    assert(NULL != device_read);
    assert(NULL != device_write);

    // Check addresses are 41 bits
    #define BIT_MASK_41 ((uint64_t)0x01fffffffffful)
    assert ( (buf_pa_in  & (~BIT_MASK_41)) == (uint64_t)0 );
    assert ( (buf_pa_out & (~BIT_MASK_41)) == (uint64_t)0 );

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

    // Load AFU parameters
    printf("%s:%d Write argument CSRs...\n", __FILE__, __LINE__);
    // Write physical addresses to AFU CSR
    res = fpgaWriteMMIO64(accel_handles[0], 0, KERNEL_ARG_DEVICE_READ_REG, buf_pa_in);
    fpga_assert(res);
    printf("%s:%d write @%08x, value = %016lx\n", __FILE__, __LINE__, KERNEL_ARG_DEVICE_READ_REG, buf_pa_in);
    res = fpgaWriteMMIO64(accel_handles[0], 0, KERNEL_ARG_DEVICE_WRITE_REG, buf_pa_out);
    fpga_assert(res);
    printf("%s:%d write @%08x, value = %016lx\n", __FILE__, __LINE__, KERNEL_ARG_DEVICE_WRITE_REG, buf_pa_out);

    // Write number of lines to loopback
    res = fpgaWriteMMIO64(accel_handles[0], 0, KERNEL_ARG_RS_LENGTH_LINES_REG, length_lines);
    fpga_assert(res);
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
    res = fpgaRegisterEvent(accel_handles[0], FPGA_EVENT_INTERRUPT, fpgaInterruptEvent, flags);
    fpga_assert(res);

    #define POLL_TIMEOUT_MS 1000
    // Pass the poll file descriptor
    struct pollfd pfd;
    pfd.events = POLLIN;
    res = fpgaGetOSObjectFromEventHandle(fpgaInterruptEvent, &pfd.fd);
    fpga_assert(res);

#endif // INTERRUPT_EVENTS

    //////////////////////////////
    // Wait for AFU to be ready //
    //////////////////////////////
    // Poll on status register
    // TODO: is there a cleaner way?
    #define SLEEP_TIME_US 1000000
    printf("%s:%d Wait for !BUSY...\n", __FILE__, __LINE__);
    do {
        usleep( SLEEP_TIME_US );
        res = fpgaReadMMIO64(accel_handles[0], 0, KERNEL_STATUS, &status_val);
        fpga_assert(res);
        print_kernel_status(status_val);
    } while ( status_val & KERNEL_REGISTER_MAP_BUSY_MASK );

    ////////////////////////
    // Start and wait AFU //
    ////////////////////////
    // Writing a '1' into the start register
    printf("%s:%d Write to START ...\n", __FILE__, __LINE__);
    res = fpgaWriteMMIO32(accel_handles[0], 0, KERNEL_START, KERNEL_START_VALUE);
    fpga_assert(res);
    printf("%s:%d write @%08x, value = %016x\n", __FILE__, __LINE__, KERNEL_START, KERNEL_START_VALUE);

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

    // Clear interrupt
    printf("%s:%d Read from KERNEL_CLEAR_INTERRUPT...\n", __FILE__, __LINE__);
    res = fpgaReadMMIO64(accel_handles[0], 0, KERNEL_CLEAR_INTERRUPT, &status_val);
    fpga_assert(res);
    printf("%s:%d read @%08x, value = %016lx\n", __FILE__, __LINE__, KERNEL_CLEAR_INTERRUPT, status_val);

#else // !INTERRUPT_EVENTS
    // Active polling on AFU
    printf("%s:%d Wait for DONE...\n", __FILE__, __LINE__);
    do {
        usleep( SLEEP_TIME_US );
        res = fpgaReadMMIO64(accel_handles[0], 0, KERNEL_STATUS, &status_val);
        fpga_assert(res);
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

cleanup_fpgaReleaseBuffer:
    // Release I/O buffers
    res = fpgaReleaseBuffer(accel_handles[0], wsid_in);
    fpga_assert(res);
    res = fpgaReleaseBuffer(accel_handles[0], wsid_out);
    fpga_assert(res);

    // Clean-up
cleanup_fpgaUnmapMMIO:
    // Unmap MMIO space
    if ( !is_ase_sim ) {
        res = fpgaUnmapMMIO(accel_handles[0], 0);
        fpga_assert(res);
    }

cleanup_fpgaUnregisterEvent:
#ifdef INTERRUPT_EVENTS
    // Cleanup event accel_handle			
    if ( fpgaInterruptEvent != NULL ) {			
        res = fpgaUnregisterEvent(accel_handles[0], FPGA_EVENT_INTERRUPT, fpgaInterruptEvent);
        fpga_assert(res);
        res = fpgaDestroyEventHandle(&fpgaInterruptEvent);
        fpga_assert(res);
    }
#endif // !INTERRUPT_EVENTS

cleanup_fpgaClose:
    res = fpgaClose(accel_handles[0]);
    fpga_assert(res);

    return 0;

out_exit:
	return res;
}
