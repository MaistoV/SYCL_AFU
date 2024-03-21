#include "sycl_afu_utils.h"

fpga_result connect_to_matching_accels(
                           const char *accel_uuid,
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
            printf("%s:%d Mapping MMIO space\n", __FILE__, __LINE__);
            res = fpgaMapMMIO(accel_handles[i], 0, &ptr); // Not supported by ASE
            assert(ptr);
            assert(FPGA_OK == res);
        } 

        // Reset AFU
        // Not supported by vfio plugin
        // res = fpgaReset( accel_handles[i] );
        // assert(FPGA_OK == res);

        // AFU reset via CSR
        printf("%s:%d Reset AFU via CSR write...\n", __FILE__, __LINE__);
        res = fpgaWriteMMIO64(accel_handles[i], 0, AFU_RESET, AFU_RESET_VALUE);
        assert(FPGA_OK == res);
        printf("%s:%d write @%08x, value = %016lx\n", __FILE__, __LINE__, AFU_RESET, AFU_RESET_VALUE);
        
        ////////////////////////////////
        // Debug reads from DFL CSRs
        ////////////////////////////////
        debug_read_dfl( accel_handles[i] );
    }
    *num_handles = num_found;
    if (0 != num_found) res = FPGA_OK;

  out_destroy:
    fpgaDestroyProperties(&filter);

    return res;
}


volatile void* alloc_buffer(fpga_handle accel_handle,
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
    
    printf("%s:%d io_addr %016p:\n", __FILE__, __LINE__, *io_addr );
    printf("%s:%d buf %016p:\n", __FILE__, __LINE__, buf );

    return buf;
}

void debug_read_dfl( fpga_handle accel_handle ) {
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
}
