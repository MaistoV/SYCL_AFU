//==============================================================
// Copyright Intel Corporation
//
// SPDX-License-Identifier: MIT
// =============================================================

#ifdef MULTI_ERASURE_SIMPLE
	#error "This source does not support multi-erasure!"
#endif

int usage( char** argv ) {
	fprintf(stderr,
		"Usage: %s [options]\n"
		"  -h		  	Print this help\n"
		"  -r <seed>	Seed for PRNG for randomized data\n"
		"  -e <0|1>		Perform encoding with ISA-L\n"
		"  -d <0|1>		Perform decoding with ISA-L\n"
		"  -m <0|1>		Measure reconstruction latency\n"
		"  -o <dir>		Output directory for latency measures (ignored for -m=0)\n"
		"  -x <0|1>		Decode once each cell and exit\n"
		"  -c <value>	Decode at most <value> cells and exit\n"
		"  -f <value>	PCIe address in SSSS:BB:DD.F format\n"
		"  -l <value>	Cell length in bytes (positive multiple of 64B)\n"
		, argv[0]
	);
	exit(0);
}

// Number of erasures for this test
#define ONE_ERASURE 1

int main(int argc, char *argv[]) {
	// Default params
	int ret_val = 0;
	unsigned int prng_seed = 54656;
	int encode_isal = 0;
	int decode_isal = 0;
	int decode_once = 0;
	int measure_latency = 0;
	unsigned int max_reconstruction = -1; // unlimited
	unsigned int cell_length = CELL_LENGTH_DEFAULT;
	unsigned long max_permutations = 0;

	// Default 0000:01:00.1
	OPAE_SIMPLE_WRAPPER_pcie_sbdf_t	pcie_sbdf = {
								.segment 	= 0,
								.bus 		= 1,
								.device 	= 0,
								.function 	= 1
							};

	// Utility string
	char tmp_string[256];
	// For latency measurement
	char filename[256];
	char filedir[256] = "./";
	double time_sec = 0.;
	FILE* fd_latency;
    std::chrono::time_point<
		std::chrono::steady_clock,
		std::chrono::nanoseconds
		> start, end;

	int c;
	while ( ( c = getopt(argc, argv, "r:e:d:l:m:o:x:c:f:h") ) != -1 ) {
		switch (c) {
		case 'r':
			prng_seed = atoi(optarg);
			break;
		case 'e':
			encode_isal = atoi(optarg);
			break;
		case 'd':
			decode_isal = atoi(optarg);
			break;
		case 'l':
			cell_length = atoi(optarg);
			if ( (cell_length <= 0) || ((cell_length % LINE_BYTE_WIDTH) != 0) ) {
				fprintf(stderr, "[ERROR] Invalid cell_length\n");
				usage( argv );
			}
			break;
		case 'm':
			measure_latency = atoi(optarg);
			break;
		case 'o':
			strcpy(filedir, optarg);
			break;
		case 'x':
			decode_once = atoi(optarg);
			break;
		case 'c':
			max_reconstruction = atoi(optarg);
			break;
		case 'f':
			int ret_val_local;
			ret_val_local = OPAE_SIMPLE_WRAPPER_parse_pcie_sbdf ( 
												optarg,
												&pcie_sbdf
											);
			if ( ret_val_local != 0 ) {
				fprintf(stderr, "[ERROR] Invalid PCIe SBDF format\n");
				usage( argv );
			}
			break;
		case 'h':
		default:
			usage ( argv );
			break;
		}
	}

	// OPAE-related variables
    fpga_handle accel_handle;
    // MMIO pointers and metadata
	uint32_t mmio_num;
    volatile uint8_t * rs_erasure_input        ;
	volatile uint8_t * reconstructed_blocks_out;
    uint64_t wsid_in, wsid_out;
    uint64_t buf_pa_in, buf_pa_out;
    // FPGA error code
    volatile fpga_result res = FPGA_OK;
	// FPGA interrupts
    fpga_event_handle fpgaInterruptEvent;
	// MMIO mapped pointer
    volatile uint64_t * mmio_ptr;
	// CSR word for AFU
	rs_erasure_csr_t rs_erasure_csr;
	// Cell length is constant across AFU calls in this test
	rs_erasure_csr.cell_length_byte_width	= cell_length / LINE_BYTE_WIDTH;

	// Skip FPGA setup if only using ISA-L
	if ( !(encode_isal && decode_isal) ) {

		//////////////////////////////////
		// Discover/Grab FPGA Resources //
		//////////////////////////////////
		res = OPAE_SIMPLE_WRAPPER_init ( 
									&accel_handle, 
									AFU_ACCEL_UUID,
									(volatile uint64_t**)&mmio_ptr,
									pcie_sbdf,
									&mmio_num
								);
		fpga_assert(res);

		if ( getenv("WITH_ASE") != NULL ) {
			printf("   *** ASE only detects a single AFU (port 0) ***\n");
		}

		
		///////////////////////////
		// Allocate MMIO buffers //
		///////////////////////////
		rs_erasure_input         = (volatile uint8_t*) OPAE_SIMPLE_WRAPPER_allocate_io_buffer (
																					accel_handle,
																					RS_INPUT_SIZE (cell_length),
																					&wsid_in,
																					&buf_pa_in
																				);
		reconstructed_blocks_out = (volatile uint8_t*) OPAE_SIMPLE_WRAPPER_allocate_io_buffer (
																					accel_handle,
																					RS_OUTPUT_SIZE(cell_length, ONE_ERASURE),
																					&wsid_out,
																					&buf_pa_out
																				);
		// Check pointers
		assert(NULL != rs_erasure_input		   );
		assert(NULL != reconstructed_blocks_out);
							
		/////////////////////////
		// Load AFU parameters //
		/////////////////////////
		// Write physical address to AFU CSR
		OPAE_SIMPLE_WRAPPER_mmio64_write ( accel_handle, mmio_num, mmio_ptr, KERNEL_ARG_DEVICE_READ_REG, buf_pa_in );
		printf("%s:%d write @%x, value = 0x%lx\n", __FILE__, __LINE__, KERNEL_ARG_DEVICE_READ_REG, buf_pa_in);

		// Write physical address to AFU CSR
		OPAE_SIMPLE_WRAPPER_mmio64_write ( accel_handle, mmio_num, mmio_ptr, KERNEL_ARG_DEVICE_WRITE_REG, buf_pa_out );
		printf("%s:%d write @%x, value = 0x%lx\n", __FILE__, __LINE__, KERNEL_ARG_DEVICE_WRITE_REG, buf_pa_out);
	}
	
	// Seed the PRNG
	srand(prng_seed);

	// Allocate coding matrices
	uint8_t encode_matrix 	[RS_M * RS_K];		// Coefficient matrices
	uint8_t g_tbls			[RS_K * RS_P * 32];	// Intermediate table for ISA-L
	uint8_t *cell_ptrs	 	[RS_M];				// Cells buffer pointers
	uint8_t *recover_outp	[RS_P];				// Reconstructed cells
	uint8_t *recover_srcs	[RS_K];

	// Allocate the src & parity buffers
	for ( unsigned int i = 0; i < RS_M; i++ ) {
		if (NULL == (cell_ptrs[i] = (uint8_t*)malloc(cell_length))) {
			printf("%s:%d Test failure! Error with malloc\n", __FILE__, __LINE__);
			return -1;
		}
	}

	// Allocate buffers for recovered data
	for ( unsigned int i = 0; i < RS_P; i++ ) {
		if (NULL == (recover_outp[i] = (uint8_t*)malloc(cell_length))) {
			printf("%s:%d Test failure! Error with malloc\n", __FILE__, __LINE__);
			return -1;
		}
	}

	// Fill sources with random data
	for ( unsigned int i = 0; i < RS_K; i++ ) {
		for ( unsigned int l = 0; l < cell_length; l++ ) {
			cell_ptrs[i][l] = rand();
		}
	}

	// Debug cell_ptrs
#ifdef DEBUG
	printf("%s:%d: cell_ptrs\n", __FILE__, __LINE__);
	for ( unsigned int i = 0; i < RS_K; i++ ) {
		print_contiguous_cell(stdout, cell_ptrs[i], ONE_ERASURE, cell_length, LINE_BYTE_WIDTH );
	}
	printf("\n");
#endif


	// Generate encode matrix for any ISA-L utilization
	if ( encode_isal || decode_isal ) {
		// Pick an encode matrix. A Cauchy matrix is a good choice as even
		// large RS_K are always invertable keeping the recovery rule simple.
		gf_gen_cauchy1_matrix(encode_matrix, RS_M, RS_K);
	#ifdef DEBUG	
		print_matrix_2d(stdout, RS_M, RS_K, encode_matrix, "encode_matrix ");
	#endif
	}

	printf("%s:%d: Encoding parity cells for RS[%d:%d] cell_length=%d, using %s\n",
		 __FILE__, __LINE__, RS_K, RS_P, cell_length, (encode_isal) ? "ISA-L" : "SYCL_AFU kernel");
		
	// Encode with ISA-L
	if ( encode_isal ) {
		// Generate g_tbls
		ec_init_tables(RS_K, RS_P, &encode_matrix[RS_K * RS_K], g_tbls);
		// Generate EC parity blocks from sources
		ec_encode_data(cell_length, RS_K, RS_P, g_tbls, cell_ptrs, &(cell_ptrs[RS_K]));
	} // encode_isal
	// Encode with rs_erasure kernel
	else { // !encode_isal
		// Rearrange input in contiguous memory
		for ( unsigned int i = 0; i < RS_K; i++ ) {
			memcpy(((uint8_t(*)[cell_length])rs_erasure_input)[i], cell_ptrs[i], sizeof(uint8_t) * cell_length );
		}

	// Debug rs_erasure_input
	#ifdef DEBUG
		printf("%s:%d: rs_erasure_input\n", __FILE__, __LINE__);
		print_contiguous_cell(stdout, (uint8_t*)rs_erasure_input, RS_K, cell_length, LINE_BYTE_WIDTH );
	#endif

		// Encode cells RS_K+1, RS_K+2, ..., RS_K+RS_P
		for ( unsigned int e = 0; e < RS_P; e++ ){
			printf("%s:%d: Encoding parity cell %d [%d/%d] with SYCL_AFU kernel\n", __FILE__, __LINE__, e, e+1, RS_P);

			// Write input
			rs_erasure_csr.survived_cells			= RS_PATTERN_MASK & ((1 << RS_K) -1); // Bitmask for first k blocks
			rs_erasure_csr.erasure_pattern			= erasure_patterns[RS_K + e];

			#ifdef DEBUG
				printf("%s:%d: rs_erasure_csr.survived_cells  0x%04x\n", __FILE__, __LINE__, rs_erasure_csr.survived_cells);
				printf("%s:%d: rs_erasure_csr.erasure_pattern 0x%04x\n", __FILE__, __LINE__, rs_erasure_csr.erasure_pattern);
			#endif

			// Call to FPGA AFU
			res = OPAE_SIMPLE_WRAPPER_call_afu (
												accel_handle,
												rs_erasure_csr.erasure_pattern,
												rs_erasure_csr.survived_cells,
												cell_length,
												OSW_SLEEP_TIME_US,
												&fpgaInterruptEvent,
												false, // Don't measure here
												mmio_ptr,
												mmio_num,
												NULL	// Don't pass any fd	
										);
			fpga_assert(res);

		#ifdef DEBUG			
			printf("%s:%d: reconstructed_blocks_out:\n", __FILE__, __LINE__);
			print_contiguous_cell(stdout, (uint8_t*)reconstructed_blocks_out, ONE_ERASURE, cell_length, LINE_BYTE_WIDTH );
		#endif

			// Pack results in cells buffer
			// Always read from first reconstructed block at index 0
			memcpy(cell_ptrs[e + RS_K], (const void*)reconstructed_blocks_out, sizeof(uint8_t) * cell_length );
		}	

	} // !encode_isal
	
// Debug Complete cell array
#ifdef DEBUG
	printf("%s:%d: Complete cell array:\n", __FILE__, __LINE__);
	for ( unsigned int i = 0; i < RS_K + RS_P; i++ ) {
		print_contiguous_cell(stdout, (uint8_t*)cell_ptrs[i], 1, cell_length, LINE_BYTE_WIDTH );
	}
#endif

	printf("%s:%d: Decoding/Reconstructing blocks RS[%d:%d] cell_length=%d, using %s\n",
		 __FILE__, __LINE__, RS_K, RS_P, cell_length, (decode_isal) ? "ISA-L" : "SYCL_AFU kernel");

	int num_vectors_per_erasure_pattern = compute_num_vectors_per_erasure_pattern (RS_K, RS_P);

	max_permutations = compute_1_erasure_patterns( RS_K, RS_P );

	// Prepare latency measurements
    if ( measure_latency ) {
		// Decode cell_length for Bytes, KBs or MBs
		unsigned int cell_length_byte;
		char cell_length_byte_power[3];
		decode_cell_length ( cell_length_byte_power, &cell_length_byte, cell_length );

		#define BASE_FILENAME "latency"
		// Open output file
		sprintf( filename, "%s_%d_%d_%d%s_%s.txt", BASE_FILENAME, RS_K, RS_P, cell_length_byte, cell_length_byte_power, (decode_isal) ? "ISA-L" : "SYCL_AFU");
		strcpy( tmp_string, filedir );
		strcat( tmp_string, "/" );
		strcat( tmp_string, filename );
		fd_latency = fopen(tmp_string, "a");
		printf("%s:%d: Appending data on file %s\n", __FILE__, __LINE__, tmp_string);
	}
	
	// Loop over all possible RS_K:RS_P permutations
	int reconstruction_count = 0;
	for ( unsigned int permutation_index = 0; permutation_index < max_permutations; permutation_index++ ) {
		for ( unsigned int survival_index = 0; 
				(survival_index < num_vectors_per_erasure_pattern) & (reconstruction_count < max_reconstruction); 
				survival_index++ ) {
			printf("%s:%d: Reconstructing cell %d [%d/%lu] with survival pattern [%d/%d], count %d\n",
				__FILE__, __LINE__, permutation_index, permutation_index+1, max_permutations,
					survival_index+1, num_vectors_per_erasure_pattern, reconstruction_count);

			// Increment counter
			reconstruction_count++;

			// Decode with ISA-L
			if ( decode_isal ) {
			#ifdef DEBUG	
				print_matrix_2d(stdout, ONE_ERASURE, RS_K, (uint8_t*)(decode_matrix_rom[permutation_index]), "decode_matrix_rom ");
				print_matrix_2d(stdout, 1, RS_K, (uint8_t*)decode_index[permutation_index][survival_index], "decode_index[permutation_index]");
			#endif

				// Pack recovery array pointers as list of valid cells
				for ( unsigned int i = 0; i < RS_K; i++ ){
					recover_srcs[i] = cell_ptrs[decode_index[permutation_index][survival_index][i]];
				}

				// Start measure by macro
				if ( measure_latency ) {
					MEASURE_LATENCY_START(start);
				}

				// Recover data
				uint8_t* decode_matrix = (uint8_t*)decode_matrix_rom[permutation_index*num_vectors_per_erasure_pattern + survival_index];
				ec_init_tables(RS_K, ONE_ERASURE, decode_matrix, g_tbls);
				ec_encode_data(cell_length, RS_K, ONE_ERASURE, g_tbls, (unsigned char **)recover_srcs, (unsigned char **)recover_outp);

				// End measure by macro
				if ( measure_latency ) {
					MEASURE_LATENCY_END_AND_PRINT(start, time_sec, fd_latency);
				}
				
			#ifdef DEBUG			
				printf("%s:%d: recover_outp:\n", __FILE__, __LINE__);
				print_contiguous_cell(stdout, (uint8_t*)recover_outp[0], ONE_ERASURE, cell_length, LINE_BYTE_WIDTH );
			#endif

			} // decode_isal
			else { // !decode_isal
				// Decode with rs_erasures
				// Rearrange input in contiguous memory
				for ( unsigned int i = 0; i < RS_K; i++ ) {
					memcpy(((uint8_t(*)[cell_length])rs_erasure_input)[i], cell_ptrs[decode_index[permutation_index][survival_index][i]], cell_length); // copy buffer
				}

			#ifdef DEBUG
				printf("%s:%d: rs_erasure_input:\n", __FILE__, __LINE__);
				print_contiguous_cell(stdout, (uint8_t*)rs_erasure_input, RS_K, cell_length, LINE_BYTE_WIDTH );
			#endif

				// Write input
				rs_erasure_csr.erasure_pattern	= erasure_patterns[permutation_index];
				rs_erasure_csr.survived_cells	= decode_index_bitstring[permutation_index][survival_index];

			#ifdef DEBUG
				printf("%s:%d: rs_erasure_csr.survived_cells  0x%04x\n", __FILE__, __LINE__, rs_erasure_csr.survived_cells);
				printf("%s:%d: rs_erasure_csr.erasure_pattern 0x%04x\n", __FILE__, __LINE__, rs_erasure_csr.erasure_pattern);
			#endif
				
				// Call to FPGA AFU
				res = OPAE_SIMPLE_WRAPPER_call_afu (
													accel_handle,
													rs_erasure_csr.erasure_pattern,
													rs_erasure_csr.survived_cells,
													cell_length,
													OSW_SLEEP_TIME_US,
													&fpgaInterruptEvent,
													measure_latency,
													mmio_ptr,
													mmio_num,
													fd_latency
											);
				fpga_assert(res);
	
			#ifdef DEBUG			
				printf("%s:%d: reconstructed_blocks_out:\n", __FILE__, __LINE__);
				print_contiguous_cell(stdout, (uint8_t*)reconstructed_blocks_out, ONE_ERASURE, cell_length, LINE_BYTE_WIDTH );
			#endif

				// Read data, always from first output cell
				memcpy(recover_outp[0], (const void*)reconstructed_blocks_out, cell_length); // copy buffer
				
			} // !decode_isal

			// Check that recovered buffers are the same as original
			ret_val = memcmp(recover_outp[0], cell_ptrs[permutation_index], cell_length);
			if ( ret_val ) {
				printf("%s:%d: Fail reconstruction %d, cell %d\n", __FILE__, __LINE__, 0, permutation_index);

				// Debug cell_ptrs
			#ifdef DEBUG
				printf("%s:%d: Expected:\n", __FILE__, __LINE__);
				print_contiguous_cell(stdout, (uint8_t*)cell_ptrs[permutation_index], 1, cell_length, LINE_BYTE_WIDTH );
				printf("%s:%d: Given:\n", __FILE__, __LINE__);
				print_contiguous_cell(stdout, (uint8_t*)recover_outp[0], 1, cell_length, LINE_BYTE_WIDTH );
			#endif
					return -1;
			}
			
			// Break out of the survival_index loop
			if ( decode_once ) {
				break;
			}

		}  // survival_index over num_vectors_per_erasure_pattern
		
		// Break out of the permutation_index loop
		if ( decode_once ) {
			break;
		}

	} // permutation_index over max_permutations

	// Close file
	if ( measure_latency ) {
		fclose(fd_latency);
		printf("%s:%d: New latency data appended on file %s\n", __FILE__, __LINE__, tmp_string);
	}

	// Skip FPGA cleanup if only using ISA-L
	if ( !(encode_isal && decode_isal) ) {
		//////////////
		// Clean up //
		//////////////
		res = OPAE_SIMPLE_WRAPPER_cleanup (
											accel_handle,
											mmio_num,
											&fpgaInterruptEvent,
											wsid_in,
											wsid_out
										);
		fpga_assert(res);
	}

	// Test summary
	printf("%s:%d: Test passed\n RS[%d:%d]\n cell_length=%d,\n encodind with %s,\n decoding with %s,\n PRNG seed=%u\n",
		 __FILE__, __LINE__, RS_K, RS_P, cell_length,
		 (encode_isal) ? "ISA-L" : "SYCL_AFU kernel",
		 (decode_isal) ? "ISA-L" : "SYCL_AFU kernel",
		 prng_seed
		 );

	return ret_val;
}
