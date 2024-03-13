// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <uuid/uuid.h>

#include <opae/fpga.h>

// State from the AFU's JSON file, extracted using OPAE's afu_json_mgr script
#include "afu_json_info.h"

// Register map emitted for DFL
#include "sycl_afu_regmap.h"
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
    fpga_result r;

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
    r = fpgaEnumerate(&filter, 1, accel_tokens, *num_handles, &num_matches);
    if (*num_handles > num_matches)
        *num_handles = num_matches;

    if ((FPGA_OK != r) || (num_matches < 1))
    {
        fprintf(stderr, "Accelerator %s not found!\n", accel_uuid);
        goto out_destroy;
    }

    // Open accelerators
    uint32_t num_found = 0;
    for (uint32_t i = 0; i < *num_handles; i += 1)
    {
        r = fpgaOpen(accel_tokens[i], &accel_handles[num_found], 0);
        if (FPGA_OK == r)
        {
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
    }
    *num_handles = num_found;
    if (0 != num_found) r = FPGA_OK;

  out_destroy:
    fpgaDestroyProperties(&filter);

    return r;
}


//
// Allocate a buffer in I/O memory, shared with the FPGA.
//
static volatile void* alloc_buffer(fpga_handle accel_handle,
                                   ssize_t size,
                                   uint64_t *wsid,
                                   uint64_t *io_addr) {
    fpga_result r;
    volatile void* buf;

    r = fpgaPrepareBuffer(accel_handle, size, (void*)&buf, wsid, 0);
    if (FPGA_OK != r) return NULL;

    // Get the physical address of the buffer in the accelerator
    r = fpgaGetIOAddress(accel_handle, *wsid, io_addr);
    assert(FPGA_OK == r);

    return buf;
}


int main(int argc, char *argv[]) {
    static const uint32_t max_handles = 32;
    fpga_handle accel_handles[max_handles];
    uint32_t num_handles = max_handles;
    volatile char *buf;
    uint64_t wsid_in, wsid_out;
    uint64_t buf_pa_in, buf_pa_out;
    bool is_ase_sim = false;
    fpga_result r;

    // Find and connect to the accelerators
    r = connect_to_matching_accels(AFU_ACCEL_UUID, &num_handles, accel_handles,
                                   &is_ase_sim);
    if ((r != FPGA_OK) || (0 == num_handles))
        exit(1);

    if (is_ase_sim)
    {
        printf("   *** ASE only detects a single AFU (port 0) ***\n");
    }

    printf("Found %d instance(s) of AFU:\n\n", num_handles);

    fpga_result res = FPGA_OK;

    #define SIZE 64
    
    volatile uint8_t * rs_erasure_input        ;
	volatile uint8_t * reconstructed_blocks_out;
    
    for (uint32_t i = 0; i < num_handles; i += 1)
    {
        printf("Handle %d\n", i);

        // Allocate a single page memory buffer
        rs_erasure_input = (volatile char*)alloc_buffer(accel_handles[i], getpagesize(),
                                           &wsid_in, &buf_pa_in);
        // Allocate a single page memory buffer
        reconstructed_blocks_out = (volatile char*)alloc_buffer(accel_handles[i], getpagesize(),
                                           &wsid_out, &buf_pa_out);

        assert(NULL != rs_erasure_input);
        assert(NULL != reconstructed_blocks_out);

        uint64_t data;
        
        if ( accel_handles[i] == NULL ) {									
            return FPGA_INVALID_PARAM;
        }

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
        
        ////////////////////////////////
        // Load AFU parameters
        ////////////////////////////////
        #define KERNEL_ARG_DEVICE_READ_REG ZTS11RSERASUREID_REGISTER_MAP_ARG_ARG_DEVICE_READ_REG
        #define KERNEL_ARG_DEVICE_WRITE_REG ZTS11RSERASUREID_REGISTER_MAP_ARG_ARG_DEVICE_WRITE_REG
        #define KERNEL_ARG_RS_ERASURE_CSR_REG ZTS11RSERASUREID_REGISTER_MAP_ARG_ARG_RS_ERASURE_CSR_REG
        
        res = fpgaWriteMMIO64(accel_handles[i], 0, KERNEL_ARG_DEVICE_READ_REG, buf_pa_in);
        printf("%s:%d write @%x, value = %x\n", __FILE__, __LINE__, KERNEL_ARG_DEVICE_READ_REG, buf_pa_in);
        printf("%s:%d res = %d\n", __FILE__, __LINE__, res);

        res = fpgaWriteMMIO64(accel_handles[i], 0, KERNEL_ARG_DEVICE_WRITE_REG, buf_pa_out);
        printf("%s:%d write @%x, value = %x\n", __FILE__, __LINE__, KERNEL_ARG_DEVICE_WRITE_REG, buf_pa_out);
        printf("%s:%d res = %d\n", __FILE__, __LINE__, res);

        // Serialize CSR inputs
        uint64_t rs_erasure_csrs = 0;
        uint64_t erasure_pattern_64 	= 0b01000;
        uint64_t survived_cells_64 		= 0b00111;
        uint64_t cell_length_64			= 2;
        rs_erasure_csrs |= erasure_pattern_64 	<< 0u ;
        rs_erasure_csrs |= survived_cells_64 	<< 16u;
        rs_erasure_csrs |= cell_length_64		<< 32u;

        res = fpgaWriteMMIO64(accel_handles[i], 0, KERNEL_ARG_RS_ERASURE_CSR_REG, rs_erasure_csrs);
        printf("%s:%d write @%x, value = %d\n", __FILE__, __LINE__, KERNEL_ARG_RS_ERASURE_CSR_REG, rs_erasure_csrs);
        printf("%s:%d res = %d\n", __FILE__, __LINE__, res);
        
		// printf("%s:%d: rs_erasure_input\n", __FILE__, __LINE__);
        // for ( unsigned int j = 0; j < (SIZE * RS_K); j++ ){
        //     rs_erasure_input[j] = rand();
        //     printf("%02x ", rs_erasure_input[j]);
        // }
        // printf("\n");

        ////////////////////////////////
        // Wait for AF to be ready
        ////////////////////////////////
        // Poll on busy register
        // TODO: is there a cleaner way?
        uint64_t status_val;		
        #define SLEEP_TIME_US 10000
        #define KERNEL_STATUS ( ZTS11RSERASUREID_REGISTER_MAP_STATUS_REG )
        do {
            usleep( SLEEP_TIME_US );
            res = fpgaReadMMIO64(accel_handles[i], 0, KERNEL_STATUS, &status_val);
            if ( res != FPGA_OK ) {
                printf("%s:%d res = %s\n", __FILE__, __LINE__, fpgaErrStr(res));
            }            
            printf("%s:%d status_val = %0lx\n", __FILE__, __LINE__, status_val);
        } while ( status_val & KERNEL_REGISTER_MAP_BUSY_MASK );
			
        // Start and wait AFU
        // fpga_result start_and_wait_afu(fpga_handle afc_handle, struct pollfd *pfd, int *poll_res)
        // TODO: automate the setting of ZTS11RSERASUREID_REGISTER_MAP_OFFSET (default is zero)
        #define KERNEL_START                 (0x8            + ZTS11RSERASUREID_REGISTER_MAP_OFFSET)
        #define POLL_TIMEOUT_MS 1000
        /* Start the AFU by writing a '1' into the valid_in bit */
        res = fpgaWriteMMIO64(accel_handles[i], 0, KERNEL_START, (uint32_t)1u);
        if ( res != FPGA_OK ) {
            printf("%s:%d res = %s\n", __FILE__, __LINE__, fpgaErrStr(res));
            return res;
        }
        // Active polling on AFU
        // TODO: switch to events
        do {
            usleep( SLEEP_TIME_US );
            res = fpgaReadMMIO64(accel_handles[i], 0, KERNEL_STATUS, &status_val);
            if ( res != FPGA_OK ) {
                printf("%s:%d res = %s\n", __FILE__, __LINE__, fpgaErrStr(res));
            }
            printf("%s:%d status_val = %0lx\n", __FILE__, __LINE__, status_val);
        } while ( !(status_val & KERNEL_REGISTER_MAP_DONE_MASK) );
		
        
        // Check output buffer
		printf("%s:%d: reconstructed_blocks_out\n", __FILE__, __LINE__);
        for ( unsigned int j = 0; j < SIZE; j++ ){
            printf("%02x ", reconstructed_blocks_out[j]);
        }
        printf("\n");

        // Clean up
        fpgaReleaseBuffer(accel_handles[i], wsid_in);
        fpgaReleaseBuffer(accel_handles[i], wsid_out);
        fpgaClose(accel_handles[i]);
    }

    return 0;

out_exit:
	return res;
}
