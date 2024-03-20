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

// Macros
#define CACHELINE_BYTES         64                              // Number of bytes of a cache line
#define CL_ALIGN(phy_addr)      (phy_addr / CACHELINE_BYTES)    // Cache-line aligned physical address
#define SIZE_BUFFERS(n)         (CACHELINE_BYTES * (n))         // Size of I/O buffers in cache lines

// Debug reads from DFL and Kernel CSRs
void debugReadMMIO( fpga_handle accel_handle ) {
    fpga_result res = FPGA_OK;
    uint64_t data = 0;
    // DFL
    res = fpgaReadMMIO64(accel_handle, 0, AFU_DFH_REG, &data);
    printf("AFU_DFH_REG = %016lx\n", data);
    res = fpgaReadMMIO64(accel_handle, 0, AFU_ID_LO, &data);
    printf("AFU ID LO = %016lx\n", data);
    res = fpgaReadMMIO64(accel_handle, 0, AFU_ID_HI, &data);
    printf("AFU ID HI = %016lx\n", data);
    res = fpgaReadMMIO64(accel_handle, 0, AFU_NEXT, &data);
    printf("AFU NEXT = %016lx\n", data);
    res = fpgaReadMMIO64(accel_handle, 0, AFU_RESERVED, &data);
    printf("AFU RESERVED = %016lx\n", data);
    // SYCL Kernel
    res = fpgaReadMMIO64(accel_handle, 0, KERNEL_STATUS, &data);
    printf("AFU KERNEL_STATUS = %016lx\n", data);
    // res = fpgaReadMMIO32(accel_handle, 0, KERNEL_START, &data);
    // printf("AFU KERNEL_START = %016lx\n", data);
    res = fpgaReadMMIO64(accel_handle, 0, KERNEL_FINISH_COUNTER, &data);
    printf("AFU KERNEL_FINISH_COUNTER = %016lx\n", data);
    res = fpgaReadMMIO64(accel_handle, 0, KERNEL_ARG_DEVICE_READ_REG, &data);
    printf("AFU KERNEL_ARG_DEVICE_READ_REG = %016lx\n", data);
    res = fpgaReadMMIO64(accel_handle, 0, KERNEL_ARG_DEVICE_WRITE_REG, &data);
    printf("AFU KERNEL_ARG_DEVICE_WRITE_REG = %016lx\n", data);
    res = fpgaReadMMIO64(accel_handle, 0, KERNEL_ARG_RS_LENGTH_LINES_REG, &data);
    printf("AFU KERNEL_ARG_RS_LENGTH_LINES_REG = %016lx\n", data);
}

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
    // setenv("ASE_LOG", "0", 0);
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
        uint64_t* ptr;
        // res = fpgaMapMMIO(accel_handles[i], 0, NULL); // Deprecated without a pointer
        if ( ! is_ase_sim ) {
            res = fpgaMapMMIO(accel_handles[i], 0, &ptr); // Not supported by ASE
            assert(ptr);
            assert(FPGA_OK == res);
        } 

        // Not supported by vfio plugin
        // Reset AFC 
        // res = fpgaReset( accel_handles[i] );
        // assert(FPGA_OK == res);
        
        ////////////////////////////////
        // Debug reads from DFL CSRs
        ////////////////////////////////
        debugReadMMIO( accel_handles[i] );
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
    assert(FPGA_OK == res);

    // Get the physical address of the buffer in the accelerator
    res = fpgaGetIOAddress(accel_handle, *wsid, io_addr);
    assert(FPGA_OK == res);

    return buf;
}

void print_kernel_status ( uint64_t status_val, uint8_t* file, uint32_t line) {
    printf("%s:%d status_val = %0lx\n" , file, line, status_val);
    printf("\t.done    = %lx\n", (status_val & KERNEL_REGISTER_MAP_DONE_MASK    ) >> KERNEL_REGISTER_MAP_DONE_OFFSET      );
    printf("\t.busy    = %lx\n", (status_val & KERNEL_REGISTER_MAP_BUSY_MASK    ) >> KERNEL_REGISTER_MAP_BUSY_OFFSET      );
    printf("\t.stalled = %lx\n", (status_val & KERNEL_REGISTER_MAP_STALLED_MASK ) >> KERNEL_REGISTER_MAP_STALLED_OFFSET   );
    printf("\t.unstall = %lx\n", (status_val & KERNEL_REGISTER_MAP_UNSTALL_MASK ) >> KERNEL_REGISTER_MAP_UNSTALL_OFFSET   );
    printf("\t.valid   = %lx\n", (status_val & KERNEL_REGISTER_MAP_VALID_IN_MASK) >> KERNEL_REGISTER_MAP_VALID_IN_OFFSET  );
    printf("\t.started = %lx\n", (status_val & KERNEL_REGISTER_MAP_STARTED_MASK ) >> KERNEL_REGISTER_MAP_STARTED_OFFSET   );
}

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
        assert ( (CL_ALIGN(buf_pa_in ) & (~BIT_MASK_41)) == (uint64_t)0 );
        assert ( (CL_ALIGN(buf_pa_out) & (~BIT_MASK_41)) == (uint64_t)0 );

        assert(NULL != device_read);
        assert(NULL != device_write);

        // Init input buffer
        for ( unsigned int j = 0; j < SIZE_BUFFERS(length_lines)/sizeof(uint32_t); j++ ) {
            // "SYCL" = 0x4c435953
            ((uint32_t*)device_read)[j] = (uint32_t)0x4c435953u;
        }   
        device_read[SIZE_BUFFERS(length_lines)-1] = "\0";

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
        #ifdef KERNEL_VIRT_ADDR
            // Write virtual addresses to AFU CSR
            res = fpgaWriteMMIO64(accel_handles[i], 0, KERNEL_ARG_DEVICE_READ_REG, (uint64_t)device_write);
            printf("%s:%d write @%08lx, value = %016lx\n", __FILE__, __LINE__, KERNEL_ARG_DEVICE_READ_REG, (uint64_t)device_write);
            res = fpgaWriteMMIO64(accel_handles[i], 0, KERNEL_ARG_DEVICE_WRITE_REG, (uint64_t)device_write);
            printf("%s:%d write @%08lx, value = %016lx\n", __FILE__, __LINE__, KERNEL_ARG_DEVICE_WRITE_REG, (uint64_t)device_write);
        #else // ! KERNEL_VIRT_ADDR
            // Write physical addresses to AFU CSR
            res = fpgaWriteMMIO64(accel_handles[i], 0, KERNEL_ARG_DEVICE_READ_REG, buf_pa_in);
            printf("%s:%d write @%08x, value = %016lx\n", __FILE__, __LINE__, KERNEL_ARG_DEVICE_READ_REG, buf_pa_in);
            res = fpgaWriteMMIO64(accel_handles[i], 0, KERNEL_ARG_DEVICE_WRITE_REG, buf_pa_out);
            printf("%s:%d write @%08x, value = %016lx\n", __FILE__, __LINE__, KERNEL_ARG_DEVICE_WRITE_REG, buf_pa_out);
        #endif // KERNEL_VIRT_ADDR
        assert(FPGA_OK == res);

        // Write number of lines to loopback
        res = fpgaWriteMMIO64(accel_handles[i], 0, KERNEL_ARG_RS_LENGTH_LINES_REG, length_lines);
        printf("%s:%d write @%08x, value = %016lx\n", __FILE__, __LINE__, KERNEL_ARG_RS_LENGTH_LINES_REG, length_lines);

        // Wait for kernel to be ready
        uint64_t status_val;
        #define SLEEP_TIME_US 1000000
        printf("%s:%d Wait for !BUSY...\n", __FILE__, __LINE__);
        do {
            usleep( SLEEP_TIME_US );
            res = fpgaReadMMIO64(accel_handles[i], 0, KERNEL_STATUS, &status_val);
            assert(FPGA_OK == res);
            print_kernel_status(status_val, __FILE__, __LINE__);
        } while ( status_val & KERNEL_REGISTER_MAP_BUSY_MASK );

        // Start kernel
        // Writing a '1' into the start register
        printf("%s:%d Write to START...\n", __FILE__, __LINE__);
        res = fpgaWriteMMIO32(accel_handles[i], 0, KERNEL_START, KERNEL_START_VALUE);
        assert(FPGA_OK == res);

        // Wait for kernel to complete
        printf("%s:%d Wait for DONE...\n", __FILE__, __LINE__);
        do {
            usleep( SLEEP_TIME_US );
            // Compiler fence
            res = fpgaReadMMIO64(accel_handles[i], 0, KERNEL_STATUS, &status_val);
            assert(FPGA_OK == res);
            print_kernel_status(status_val, __FILE__, __LINE__);
        } while ( !(status_val & KERNEL_REGISTER_MAP_DONE_MASK) );


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
