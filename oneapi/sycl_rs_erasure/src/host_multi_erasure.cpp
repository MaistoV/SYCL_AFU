//==============================================================
// Copyright Intel Corporation
//
// SPDX-License-Identifier: MIT
// =============================================================

#include <iostream>
#include <chrono>

#ifndef NO_SYCL
	#include <sycl/sycl.hpp>
	#include <sycl/ext/intel/fpga_extensions.hpp>
	#include "exception_handler.hpp"
#endif // !NO_SYCL


// Header for device code.
#include "rs_erasure/rs_erasure_sycl.hpp"

// For verification
#include <getopt.h>
// For ISA-L
// #include <isa-l.h>
// For compute_max_erasure_patterns, gf_gen_cauchy1_matrix, compute_num_vectors_per_erasure_pattern, gf_gen_decode_matrix_simple
#include "rs_erasure/roms/src/rs_rom_utils.h"

// Measure macros for latency
#include "measure_latency.h"

using namespace sycl;

// Utility function wrapping the complexity of the SYCL_ASP call
void RunKernel(
		int measure_latency,
		FILE* fd_latency,
		unsigned int cell_length,
		unsigned int NUM_ERASURES,
		device_read_t rs_erasure_input,
		device_write_t reconstructed_blocks_out,
		rs_erasure_csr_t rs_erasure_csr
	);

// Decode cell_length for Bytes, KBs or MBs
int decode_cell_length (
		char cell_length_byte_power[2],
		unsigned int* cell_length_byte,
		const unsigned int cell_length
		);

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
		"  -l <value>	Cell length in bytes (positive multiple of 64B)\n"
		, argv[0]
	);
	exit(0);
}

// DEBUG: make selector global for now
// Select either the FPGA emulator, FPGA simulator or FPGA device
#ifndef NO_SYCL
#if FPGA_SIMULATOR
	auto selector = sycl::ext::intel::fpga_simulator_selector_v;
#elif FPGA_HARDWARE
	auto selector = sycl::ext::intel::fpga_selector_v;
#else	// #if FPGA_EMULATOR
	auto selector = sycl::ext::intel::fpga_emulator_selector_v;
#endif
#endif // ! NO_SYCL

// Number of erasures for this test
#define NUM_ERASURES RS_P

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
	unsigned int j, j_init = 0;

	// For latency measurement
	char filename[256];
	char filedir[256] = "./";
	char tmp_string[256];
	double time_sec = 0.;
	FILE* fd_latency;
    std::chrono::time_point<
		std::chrono::steady_clock,
		std::chrono::nanoseconds
		> start, end;

	// Permutation buffers
	int c;
	while ( ( c = getopt(argc, argv, "r:e:d:l:m:o:x:c:h") ) != -1 ) {
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
		case 'h':
		default:
			usage ( argv );
			break;
		}
	}

	// Maximum number of erasure patterns
	// int max_erasure_patterns = compute_max_erasure_patterns( RS_K, RS_P, NUM_ERASURES );

	// Interface arguments for IP
	line_t* rs_erasure_input		 = (line_t*)malloc( RS_INPUT_SIZE(cell_length)					);
	line_t* reconstructed_blocks_out = (line_t*)malloc( RS_OUTPUT_SIZE(cell_length, NUM_ERASURES)	);

	// CSR input to IP under test
	rs_erasure_csr_t rs_erasure_csr;
	rs_erasure_csr.erasure_pattern	= -1;
	rs_erasure_csr.survived_cells	= -1;
	rs_erasure_csr.cell_length_byte_width = cell_length / LINE_BYTE_WIDTH;
	
	// Seed the PRNG
	srand(prng_seed);

	// Allocate coding matrices
	uint8_t encode_matrix 	[RS_M * RS_K];		// Coefficient matrices
	uint8_t g_tbls			[RS_K * RS_P * 32];	// Intermediate table for ISA-L
	uint8_t *frag_ptrs	 	[RS_M];				// Fragment buffer pointers
	uint8_t *recover_outp	[RS_P];				// Reconstructed cells
	uint8_t *recover_srcs	[RS_K];

	// Allocate the src & parity buffers
	for ( unsigned int i = 0; i < RS_M; i++ ) {
		if (NULL == (frag_ptrs[i] = (uint8_t*)malloc(cell_length))) {
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
			frag_ptrs[i][l] = rand();
		}
	}

	// Debug frag_ptrs
#ifdef DEBUG
	printf("%s:%d: frag_ptrs\n", __FILE__, __LINE__);
	for ( unsigned int i = 0; i < RS_K; i++ ) {
		for ( unsigned int l = 0; l < cell_length; l++ ) {
			printf("%02x ", frag_ptrs[i][l]);
			if ( ((l+1) % LINE_BYTE_WIDTH) == 0 ) {
				printf("\n");
			}
		}
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
		 __FILE__, __LINE__, RS_K, RS_P, cell_length, (encode_isal) ? "ISA-L" : "SYCL kernel");
	
	// Encode with ISA-L
	if ( encode_isal ) {
		// Generate g_tbls
		ec_init_tables(RS_K, RS_P, &encode_matrix[RS_K * RS_K], g_tbls);
		// Generate EC parity blocks from sources
		ec_encode_data(cell_length, RS_K, RS_P, g_tbls, frag_ptrs, &(frag_ptrs[RS_K]));
	} // encode_isal
	// Encode with rs_erasure kernel
	else { // !encode_isal
		// Rearrange input in contiguous memory
		for ( unsigned int i = 0; i < RS_K; i++ ) {
			for ( int l = 0; l < cell_length; l++ ) {
				((uint8_t(*)[cell_length])rs_erasure_input)[i][l] = frag_ptrs[i][l];
			}
		}

	// Debug rs_erasure_input
	#ifdef DEBUG
		printf("%s:%d: rs_erasure_input\n", __FILE__, __LINE__);
		for ( unsigned int i = 0; i < RS_K; i++ ) {
			for ( unsigned int l = 0; l < cell_length; l++ ) {
				printf("%02x ", ((uint8_t(*)[cell_length])rs_erasure_input)[i][l]);
				if ( ((l+1) % LINE_BYTE_WIDTH) == 0 ) {
					printf("\n");
				}
			}
		}
		printf("\n");
	#endif

		// Encode fragments RS_K+1, RS_K+2, ..., RS_K+RS_P
		printf("%s:%d: Encoding parity cells with SYCL_ASP kernel\n", __FILE__, __LINE__);

		// Write input
		rs_erasure_csr.survived_cells	= RS_PATTERN_MASK &  ((1 << RS_K) -1); // Bitmask for first k blocks
		rs_erasure_csr.erasure_pattern	= RS_PATTERN_MASK & ~((1 << RS_K) -1); // Bitmask for last p blocks

			// Call to kernel
			// Don't measure latency for encoding
			RunKernel (
					0,			
					NULL,
					cell_length,
					NUM_ERASURES,
					rs_erasure_input,
					reconstructed_blocks_out,
					rs_erasure_csr
				);

			// Pack results in fragments buffer
		for ( unsigned int e = 0; e < NUM_ERASURES; e++ ) {
			for ( int l = 0; l < cell_length; l++ ) {
				frag_ptrs[e + RS_K][l] = ((uint8_t(*)[cell_length])reconstructed_blocks_out)[e][l];
			}
		}	
	} // !encode_isal

// Debug Complete cell array
#ifdef DEBUG
	printf("%s:%d: Complete cell array:\n", __FILE__, __LINE__);
	for ( unsigned int i = 0; i < RS_K + RS_P; i++ ) {
		for ( unsigned int l = 0; l < cell_length; l++ ) {
			printf("%02x ", frag_ptrs[i][l]);
			if ( ((l+1) % LINE_BYTE_WIDTH) == 0 ) {
				printf("\n");
			}
		}
		printf("\n");
	}
	printf("\n");
#endif

	printf("%s:%d: Decoding/Reconstructing blocks RS[%d:%d] cell_length=%d, using %s\n",
		 __FILE__, __LINE__, RS_K, RS_P, cell_length, (decode_isal) ? "ISA-L" : "SYCL_ASP kernel");

	int num_vectors_per_erasure_pattern = compute_num_vectors_per_erasure_pattern (RS_K, RS_P);

	max_permutations = compute_max_erasure_patterns( RS_K, RS_P, NUM_ERASURES );

	// Prepare latency measurements
    if ( measure_latency ) {
		// Decode cell_elgth for Bytes, KBs or MBs
		unsigned int cell_length_byte;
		char cell_length_byte_power[3];
		decode_cell_length ( cell_length_byte_power, &cell_length_byte, cell_length );

		#define BASE_FILENAME "latency"
		// Open output file
		sprintf( filename, "%s_%d_%d_%d%s_%s.txt", BASE_FILENAME, RS_K, RS_P, cell_length_byte, cell_length_byte_power, (decode_isal) ? "ISA-L" : "SYCL_ASP");
		strcpy( tmp_string, filedir );
		strcat( tmp_string, "/" );
		strcat( tmp_string, filename );
		fd_latency = fopen(tmp_string, "a");
		printf("%s:%d: Appending data on file %s\n", __FILE__, __LINE__, tmp_string);
	}
	
	// Loop over all possible RS_K:RS_P permutations
	int reconstruction_count = 0;
	printf("%s:%d: max_permutations %lu, max_reconstruction %u\n", __FILE__, __LINE__, max_permutations, max_reconstruction);

	// Start from the last group, i.e. the one with P erasures
	unsigned int p_erasure_patterns = compute_p_erasure_patterns( RS_K, RS_P );
	unsigned int erasure_pattern_offset = max_permutations - p_erasure_patterns;
	for ( unsigned int permutation_index = erasure_pattern_offset;
			(permutation_index < max_permutations) & (reconstruction_count < max_reconstruction);
			permutation_index++ ) {

		printf("%s:%d: Reconstruction [%d/%lu], permutation_index %d, count %d\n",
			__FILE__, __LINE__, permutation_index - erasure_pattern_offset +1, max_permutations - erasure_pattern_offset, permutation_index, reconstruction_count);

		// Increment counter
		reconstruction_count++;

		// Survival pattern bitstring
		uint16_t survival_pattern = (~erasure_patterns[permutation_index]) & RS_PATTERN_MASK;
		uint16_t erasure_pattern  = erasure_patterns[permutation_index] & RS_PATTERN_MASK;

	#ifdef DEBUG
		printf("%s:%d: survival_pattern 0x%04x\n", __FILE__, __LINE__, survival_pattern);
		printf("%s:%d: erasure_pattern 0x%04x\n", __FILE__, __LINE__, erasure_pattern);
	#endif

		// Pack recovery array pointers as list of valid fragments
		uint8_t survival_pattern_index [RS_M] = {0};
		uint16_t survival_pattern_copy = survival_pattern;
		for ( unsigned int i = 0; i < RS_M; i++ ) {
			survival_pattern_index[i] = (survival_pattern_copy % 2);
			survival_pattern_copy /= 2;
		}
		uint8_t erasure_pattern_index [RS_M] = {0};
		uint16_t erasure_pattern_copy = erasure_pattern;
		for ( unsigned int i = 0; i < RS_M; i++ ) {
			erasure_pattern_index[i] = (erasure_pattern_copy % 2);
			erasure_pattern_copy /= 2;
		}

		// Extract the indeces
		// NOTE: this "if high" logic could be exported in a function
		uint8_t survival_pattern_list [RS_K] = {0};
		j_init = 0;
		for ( unsigned int i = 0; i < RS_K; i++ ){			// For each survived cell
			for ( j = j_init; j < RS_M; j++ ){				// Scan the survival pattern
				if ( survival_pattern_index[j] ) { 			// if high
					survival_pattern_list[i] = j;
					break;									// Break out of this loop
				}
			}
			// Start next j iteration from next index
			j_init = j + 1;
		}
		// Same for erasure pattern
		uint8_t erasure_pattern_list [NUM_ERASURES] = {0};
		j_init = 0;
		for ( unsigned int i = 0; i < NUM_ERASURES; i++ ){			// For each survived cell
			for ( j = j_init; j < RS_M; j++ ){				// Scan the survival pattern
				if ( erasure_pattern_index[j] ) { 			// if high
					erasure_pattern_list[i] = j;
					break;									// Break out of this loop
				}
			}
			// Start next j iteration from next index
			j_init = j + 1;
		}

	#ifdef DEBUG
		printf("%s:%d: survival_pattern_index: ", __FILE__, __LINE__);
		for ( unsigned int i = 0; i < RS_M; i++ ) {
			printf("%hhu", survival_pattern_index[RS_M -i -1]);
		}
		printf("\n");
		printf("%s:%d: erasure_pattern_index: ", __FILE__, __LINE__);
		for ( unsigned int i = 0; i < RS_M; i++ ) {
			printf("%hhu", erasure_pattern_index[RS_M -i -1]);
		}
		printf("\n");
		print_matrix_2d(stdout, 1, RS_K, (uint8_t*)survival_pattern_list, "survival_pattern_list");
		print_matrix_2d(stdout, 1, NUM_ERASURES, (uint8_t*)erasure_pattern_list, "erasure_pattern_list");
	#endif

		// Decode with ISA-L
		if ( decode_isal ) {
			// Retrieve the pointers of the survived cells
			j_init = 0;
			for ( unsigned int i = 0; i < RS_K; i++ ){			// For each survived cell
				for ( j = j_init; j < RS_M; j++ ){				// Scan the survival pattern
					if ( survival_pattern_index[j] ) { 			// if high
						recover_srcs[i] = frag_ptrs[j]; 		// copy pointer
						break;									// Break out of this loop
					}
				}
				// Start next j iteration from next index
				j_init = j + 1;
			}

			// Start measure by macro
			if ( measure_latency ) {
				MEASURE_LATENCY_START(start);
			}

			uint8_t decode_matrix 	[NUM_ERASURES][RS_K];
			uint8_t temp_matrix 	[RS_M][RS_K];
			uint8_t invert_matrix 	[RS_M][RS_K];

			// Re-compute a decode matrix to regenerate all erasures from remaining frags
			// Altough the decode vectors have already been computed in DECMAT_ROMs, fetching
			// multiple vectors is come complex than just deterministically re-generating them
			ret_val = gf_gen_decode_matrix_simple (
													encode_matrix,
													(uint8_t*)decode_matrix,
													(uint8_t*)invert_matrix,
													(uint8_t*)temp_matrix,
													survival_pattern_list,
													erasure_pattern_list,
													NUM_ERASURES,
													RS_K,
													RS_M
												);
			if ( ret_val != 0 ) {
				printf("%s:%d: Fail on generating decode matrix (%d)\n", __FILE__, __LINE__, ret_val);
				return -1;
			}
		#ifdef DEBUG
			print_matrix_2d(stdout, NUM_ERASURES, RS_K, (uint8_t*)(decode_matrix), "decode_matrix ");
		#endif

			// Recover data
			ec_init_tables(RS_K, NUM_ERASURES, (unsigned char *)decode_matrix, g_tbls);
			ec_encode_data(cell_length, RS_K, NUM_ERASURES, g_tbls, (unsigned char **)recover_srcs, (unsigned char **)recover_outp);

			// End measure by macro
			if ( measure_latency ) {
				MEASURE_LATENCY_END_AND_PRINT(start, time_sec, fd_latency);
			}
		#ifdef DEBUG			
			printf("%s:%d: reconstructed_blocks_out:\n", __FILE__, __LINE__);
			for ( unsigned int i = 0; i < NUM_ERASURES; i++ ) {
				for ( int l = 0; l < cell_length; l++ ) {
					printf("%02x ", recover_outp[i][l]);
					if ( ((l+1) % LINE_BYTE_WIDTH) == 0 ) {
						printf("\n");
					}
				}
			}
			printf("\n");
		#endif

		} // decode_isal
		else { // !decode_isal
			// Decode with rs_erasures
			// Rearrange input in contiguous memory
			j_init = 0;
			for ( unsigned int i = 0; i < RS_K; i++ ){			// For each survived cell
				for ( j = j_init; j < RS_M; j++ ){				// Scan the survival pattern
					if ( survival_pattern_index[j] ) { 			// if high
						for ( int l = 0; l < cell_length; l++ ) {	// copy buffer
							((uint8_t(*)[cell_length])rs_erasure_input)[i][l] = frag_ptrs[j][l];
						}
						break;									// Break out of this loop
					}
				}
				// Start next j iteration from next index
				j_init = j + 1;
			}

		#ifdef DEBUG
			printf("%s:%d: rs_erasure_input:\n", __FILE__, __LINE__);
			for ( unsigned int i = 0; i < RS_K; i++ ) {
				for ( unsigned int l = 0; l < cell_length; l++ ) {
					printf("%02x ", ((uint8_t(*)[cell_length])rs_erasure_input)[i][l]);
					if ( ((l+1) % LINE_BYTE_WIDTH) == 0 ) {
						printf("\n");
					}
				}
				printf("\n");
			}
			printf("\n");
		#endif

			// Write input
			rs_erasure_csr.erasure_pattern	= erasure_pattern;
			// Just flip erasure_pattern
			rs_erasure_csr.survived_cells	= survival_pattern;

			// Call to kernel
			RunKernel (
					measure_latency,
					fd_latency,
					cell_length,
					NUM_ERASURES,
					rs_erasure_input,
					reconstructed_blocks_out,
					rs_erasure_csr
				);

		#ifdef DEBUG			
			printf("%s:%d: reconstructed_blocks_out:\n", __FILE__, __LINE__);
			for ( unsigned int i = 0; i < NUM_ERASURES; i++ ) {
				for ( int l = 0; l < cell_length; l++ ) {
					printf("%02x ", ((uint8_t(*)[cell_length])reconstructed_blocks_out)[i][l]);
					if ( ((l+1) % LINE_BYTE_WIDTH) == 0 ) {
						printf("\n");
					}
				}
			}
			printf("\n");
		#endif

			// Read data
			for ( unsigned int i = 0; i < NUM_ERASURES; i++ ) {
				for ( int l = 0; l < cell_length; l++ ) {
					recover_outp[i][l] = ((uint8_t(*)[cell_length])reconstructed_blocks_out)[i][l];
				}
			}
		} // !decode_isal

		// Check that recovered buffers are the same as original

		j_init = 0;
		for ( unsigned int i = 0; i < NUM_ERASURES; i++ ) {
			for ( j = j_init; j < RS_M; j++ ){	  // Scan the erasure pattern
				if ( erasure_pattern_index[j] ) { // if high
					printf("%s:%d: Checking reconstruction %u, fragment %u\n", __FILE__, __LINE__, i, j);
					// Check buffers
					ret_val = memcmp(recover_outp[i], frag_ptrs[j], cell_length);

					if ( ret_val ) {
						printf("%s:%d: Fail reconstruction %d, frag %d\n", __FILE__, __LINE__, i, j);

					// Debug frag_ptrs
					#ifdef DEBUG
						printf("%s:%d: Expected:\n", __FILE__, __LINE__);
						for ( unsigned int l = 0; l < cell_length; l++ ) {
							printf("%02x ", frag_ptrs[j][l]);
							if ( ((l+1) % LINE_BYTE_WIDTH) == 0 ) {
								printf("\n");
							}
						}
						printf("\n");

						printf("%s:%d: Given:\n", __FILE__, __LINE__);
						for ( int l = 0; l < cell_length; l++ ) {
							printf("%02x ", recover_outp[i][l]);
							if ( ((l+1) % LINE_BYTE_WIDTH) == 0 ) {
								printf("\n");
							}
						}
						printf("\n");
					#endif
						return -1;
					}

					// Break out of this j loop
					break;
				}
			}

			// Start next j iteration from next index
			j_init = j + 1;

		} // Check results
			
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

	// Test summary
	printf("%s:%d: Test passed\n RS[%d:%d]\n cell_length=%d,\n encodind with %s,\n decoding with %s,\n PRNG seed=%u\n",
		 __FILE__, __LINE__, RS_K, RS_P, cell_length,
		 (encode_isal) ? "ISA-L" : "SYCL kernel",
		 (decode_isal) ? "ISA-L" : "SYCL kernel",
		 prng_seed
		 );

	return ret_val;
}

void RunKernel(
		int measure_latency,
		FILE* fd_latency,
		unsigned int cell_length,
		unsigned int num_erasures,
		device_read_t rs_erasure_input,
		device_write_t reconstructed_blocks_out,
		rs_erasure_csr_t rs_erasure_csr
	){

#ifdef NO_SYCL
	// Start measure by macro
	std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds> start, end;
	double time_sec = 0.0;
	if ( measure_latency ) {
		MEASURE_LATENCY_START(start);
	}

	rs_erasure (
                 rs_erasure_input,
                 reconstructed_blocks_out,
                 rs_erasure_csr
                );

	// End measure by macro
	if ( measure_latency ) {
		MEASURE_LATENCY_END_AND_PRINT(start, time_sec, fd_latency);
	}

#else // ! NO_SYCL

	try {
		// Create a queue bound to the chosen device.
		// If the device is unavailable, a SYCL_ASP runtime exception is thrown.
		queue q(selector, fpga_tools::exception_handler);

		auto device = q.get_device();

		std::cout << "Running on device: "
							<< device.get_info<sycl::info::device::name>().c_str()
							<< std::endl;

		// Run kernel
		RunKernelLambda(
						q,
						measure_latency,
						fd_latency,
						num_erasures,
						rs_erasure_input,
						reconstructed_blocks_out,
						rs_erasure_csr
					);
					
		
	} catch (exception const &e) {
		// Catches exceptions in the host code
		std::cerr << "Caught a SYCL_ASP host exception:\n" << e.what() << "\n";

		// Most likely the runtime couldn't find FPGA hardware!
		if (e.code().value() == CL_DEVICE_NOT_FOUND) {
			std::cerr << "CL_DEVICE_NOT_FOUND\n";
		}
		std::terminate();

	} // try/catch
#endif // !NO_SYCL

} // RunKernel


int decode_cell_length ( char cell_length_byte_power[2], unsigned int* cell_length_byte, const unsigned int cell_length ) {
	unsigned int byte_power = 0;

	*cell_length_byte = cell_length;
	while ( (*cell_length_byte / 1024) != 0 ) {
		*cell_length_byte /= 1024;
		byte_power++;
	}

	cell_length_byte_power[1] = 'B';
	switch ( byte_power ) {
	case 0:
		cell_length_byte_power[0] = 'B';
		cell_length_byte_power[1] = '\0';
		break;
	case 1:
		cell_length_byte_power[0] = 'K';
		break;
	case 2:
		cell_length_byte_power[0] = 'M';
		break;
	default:
		printf("Error decoding cell_length, aborting");
		return -1;
		break;
	}
	cell_length_byte_power[2] = '\0';

	return 0;
};
