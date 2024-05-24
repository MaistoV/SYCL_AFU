#include "sycl_afu_utils.h"

fpga_result connect_to_matching_accels(
                           const char *accel_uuid,
                           uint32_t *num_handles,
                           fpga_handle *accel_handles,
                           bool *is_ase_sim,
                           unsigned int max_handles,
                           volatile uint64_t** mmio_ptr
                           ) {
    fpga_properties filter = NULL;
    fpga_guid guid;
    const uint32_t max_tokens = 32;
    fpga_token accel_tokens[max_tokens];
    uint32_t num_matches;
    fpga_result res;

    assert(accel_handles);

    // Limit num_handles to max_tokens. We could be smarter and dynamically
    // allocate accel_tokens.
    if (*num_handles > max_tokens)
        *num_handles = max_tokens;

    // Don't print verbose messages in ASE by default
    // setenv("ASE_LOG", "0", 0);
    *is_ase_sim = false;

    // Set up a filter that will search for an accelerator
    res = fpgaGetProperties(NULL, &filter);
    fpga_assert(res);
    res = fpgaPropertiesSetObjectType(filter, FPGA_ACCELERATOR);
    fpga_assert(res);

    // Add the desired UUID to the filter
    uuid_parse(accel_uuid, guid);
    res = fpgaPropertiesSetGUID(filter, guid);
    fpga_assert(res);

    // Search across the available FPGA contexts
    res = fpgaEnumerate(&filter, 1, accel_tokens, *num_handles, &num_matches);
    fpga_assert(res);
    if (*num_handles > num_matches) {
        *num_handles = num_matches;
    }

    if ((FPGA_OK != res) || (num_matches < 1)) {
        fprintf(stderr, "Accelerator %s not found!\n", accel_uuid);
        res = fpgaDestroyProperties(&filter);
        fpga_assert(res);
        return res;
    }

    // Open accelerators
    uint32_t num_found = 0;
    for (uint32_t i = 0; (i < *num_handles) && (i < max_handles); i += 1) {
        res = fpgaOpen(accel_tokens[i], &accel_handles[num_found], 0);
        if (FPGA_OK == res) {
            num_found += 1;

            // While the token is available, check whether it is for HW
            // or for ASE simulation, recording it so probeForASE() below
            // doesn't have to run through the device list again.
            fpga_properties accel_props;
            uint16_t vendor_id, dev_id;
            res = fpgaGetProperties(accel_tokens[i], &accel_props);
            fpga_assert(res);
            res = fpgaPropertiesGetVendorID(accel_props, &vendor_id);
            fpga_assert(res);
            res = fpgaPropertiesGetDeviceID(accel_props, &dev_id);
            fpga_assert(res);
            *is_ase_sim = (vendor_id == 0x8086) && (dev_id == 0xa5e);
        }

        res = fpgaDestroyToken(&accel_tokens[i]);
        fpga_assert(res);

        // Map MMIO address space
        if ( !( *is_ase_sim ) ) {
            printf("%s:%d Mapping MMIO space\n", __FILE__, __LINE__);
            volatile uint64_t * tmp_ptr;
            res = fpgaMapMMIO(accel_handles[i], 0, ((uint64_t **)&tmp_ptr));
            fpga_assert(res);
            assert(tmp_ptr != NULL);
            mmio_ptr[i] = tmp_ptr;
        } 

        // Reset AFU
        // Not supported by vfio plugin
        // res = fpgaReset( accel_handles[i] );
        // fpga_assert(res);

        // AFU reset via CSR
        printf("%s:%d Reset AFU via CSR write...\n", __FILE__, __LINE__);
        if ( *is_ase_sim ) {
            res = fpgaWriteMMIO64(accel_handles[i], 0, AFU_RESET, AFU_RESET_VALUE);
            fpga_assert(res);
        }
        else {
            MAPPED_MMIO(mmio_ptr[i], AFU_RESET) = AFU_RESET_VALUE;
        }
        printf("%s:%d write @%08x, value = %016lx\n", __FILE__, __LINE__, AFU_RESET, AFU_RESET_VALUE);
        
        // Enable AFU interrupts via CSR
        if ( !(*is_ase_sim) ) {
            printf("%s:%d Enable AFU interrupts via CSR write...\n", __FILE__, __LINE__);
            MAPPED_MMIO(mmio_ptr[i], AFU_IRQ_EN) = AFU_IRQ_EN_VALUE;
            printf("%s:%d write @%08x, value = %016lx\n", __FILE__, __LINE__, AFU_IRQ_EN, AFU_IRQ_EN_VALUE);
        }
        
        ///////////////////////////////
        // Debug reads from DFL CSRs //
        ///////////////////////////////
        debug_read_dfl( accel_handles[i], mmio_ptr[i], is_ase_sim );
    }
    *num_handles = num_found;
    if (0 != num_found) res = FPGA_OK;

    // Clean up
    res = fpgaDestroyProperties(&filter);
    fpga_assert(res);

    return res;
}

volatile void* alloc_buffer(
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

    printf("%s:%d io_addr %016lx:\n", __FILE__, __LINE__, *io_addr );
    printf("%s:%d buf %p:\n", __FILE__, __LINE__, buf );

    return buf;
}

void debug_read_dfl( fpga_handle accel_handle, volatile uint64_t* mmio_ptr, bool is_ase_sim ) {
	fpga_result res = FPGA_OK;
	
    // Mapped MMIO access
    if ( !is_ase_sim ) {
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

}
