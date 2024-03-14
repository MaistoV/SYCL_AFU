//==============================================================
// Copyright Intel Corporation
//
// SPDX-License-Identifier: MIT
// =============================================================

#include <iostream>
// #include <vector>

#include <sycl/sycl.hpp>
#include <sycl/ext/intel/fpga_extensions.hpp>

#include "exception_handler.hpp"

// Header for device code.
#include "rs_erasure/rs_erasure_sycl.hpp"

// For verification
#include <getopt.h>
// For ISA-L
// #include <isa-l.h>
// For compute_max_erasure_patterns, gf_gen_cauchy1_matrix, compute_num_vectors_per_erasure_pattern
#include "rs_erasure/roms/src/rs_rom_utils.h"

using namespace sycl;

// Utility function wrapping the complexity of the SYCL call
void RunKernel(
		// sycl::device_selector selector,
		unsigned int cell_length,
		line_t* rs_erasure_input,
		line_t* reconstructed_blocks_out,
		rs_erasure_csr_t rs_erasure_csr
	);

int usage( char** argv ) {
	fprintf(stderr,
		"Usage: %s [options]\n"
		"  -h		  	Print this help\n"
		"  -r <seed>	Seed for PRNG for randomized data\n"
		"  -e <0|1>		Perform encoding with ISA-L\n"
		"  -d <0|1>		Perform decoding with ISA-L\n"
		"  -l <value>	Cell length in bytes (positive multiple of 64B)\n",
		argv[0]
	);
	exit(0);
}

// DEBUG: make selector global for now
	// Select either the FPGA emulator, FPGA simulator or FPGA device
#if FPGA_SIMULATOR
	auto selector = sycl::ext::intel::fpga_simulator_selector_v;
#elif FPGA_HARDWARE
	auto selector = sycl::ext::intel::fpga_selector_v;
#else	// #if FPGA_EMULATOR
	auto selector = sycl::ext::intel::fpga_emulator_selector_v;
#endif

int main(int argc, char *argv[]) {
	int ret_val = 0;
	// Default params
	unsigned int prng_seed = 54656;
	int encode_isal = 0;
	int decode_isal = 0;
	unsigned int cell_length = CELL_LENGTH_DEFAULT;

	unsigned long max_permutations;
	if ( NUM_ERASURES == RS_P ) {
		max_permutations = compute_max_erasure_patterns( RS_K, RS_P );
	}
	else if ( NUM_ERASURES == 1 ) {
		max_permutations = RS_K + RS_P;
	}

	// Permutation buffers
	int c;
	while ( ( c = getopt(argc, argv, "r:e:d:l:h") ) != -1 ) {
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
			if ( (cell_length <= 0) || ((cell_length % CELL_BYTE_WIDTH) != 0) ) {
				usage( argv );
			}
			break;
		case 'h':
		default:
			usage ( argv );
			break;
		}
	}
	
	// Interface arguments for IP
	line_t* rs_erasure_input		 = (line_t*)malloc( RS_INPUT_SIZE(cell_length)	);
	line_t* reconstructed_blocks_out = (line_t*)malloc( RS_OUTPUT_SIZE(cell_length)	);

	// CSR input to IP under test
	rs_erasure_csr_t rs_erasure_csr;
	rs_erasure_csr.erasure_pattern	= -1;
	rs_erasure_csr.survived_cells	= -1;
	rs_erasure_csr.cell_length_byte_width = cell_length / CELL_BYTE_WIDTH;
	
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

#ifdef DEBUG
	printf("%s:%d: frag_ptrs\n", __FILE__, __LINE__);
	for ( unsigned int i = 0; i < RS_K; i++ ) {
		for ( unsigned int l = 0; l < cell_length; l++ ) {
			printf("%02x ", frag_ptrs[i][l]);
			if ( ((l+1) % CELL_BYTE_WIDTH) == 0 ) {
				printf("\n");
			}
		}
	}
	printf("\n");
#endif

	printf("%s:%d: Encoding parity cells for RS[%d:%d] cell_length=%d, using %s\n",
		 __FILE__, __LINE__, RS_K, RS_P, cell_length, (encode_isal) ? "ISA-L" : "HLS core");
	
	// Encode with ISA-L
	if ( encode_isal ) {
		// Pick an encode matrix. A Cauchy matrix is a good choice as even
		// large RS_K are always invertable keeping the recovery rule simple.
		gf_gen_cauchy1_matrix(encode_matrix, RS_M, RS_K);
	#ifdef DEBUG	
		print_matrix_2d(stdout, RS_M, RS_K, encode_matrix, "encode_matrix ");
	#endif
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

	#ifdef DEBUG
		printf("%s:%d: rs_erasure_input\n", __FILE__, __LINE__);
		for ( unsigned int i = 0; i < RS_K; i++ ) {
			for ( unsigned int l = 0; l < cell_length; l++ ) {
				printf("%02x ", ((uint8_t(*)[cell_length])rs_erasure_input)[i][l]);
				if ( ((l+1) % CELL_BYTE_WIDTH) == 0 ) {
					printf("\n");
				}
			}
		}
		printf("\n");
	#endif

		// Encode fragments RS_K+1, RS_K+2, ..., RS_K+RS_P
		for ( unsigned int i = 0; i < RS_P; i++ ){
			printf("%s:%d: Encoding parity cell %d [%d/%d] with HLS\n", __FILE__, __LINE__, i, i+1, RS_P);

			// Write input
			rs_erasure_csr.erasure_pattern	= permutations_pattern[RS_K + i];
			rs_erasure_csr.survived_cells	= (~rs_erasure_csr.erasure_pattern) & ((1 << RS_K) -1); // Bitmask for first k blocks

			// Call to kernel
			RunKernel (
					// selector,
					cell_length,
					rs_erasure_input,
					reconstructed_blocks_out,
					rs_erasure_csr
				);

			// Pack results in fragments buffer
			for ( unsigned int e = 0; e < NUM_ERASURES; e++ ) {
				for ( int l = 0; l < cell_length; l++ ) {
					frag_ptrs[i + RS_K][l] = ((uint8_t(*)[cell_length])reconstructed_blocks_out)[e][l];
				}
			}
		}
	} // !encode_isal
#ifdef DEBUG
	printf("%s:%d: Complete cell array:\n", __FILE__, __LINE__);
	for ( unsigned int i = 0; i < RS_K + RS_P; i++ ) {
		for ( unsigned int l = 0; l < cell_length; l++ ) {
			printf("%02x ", frag_ptrs[i][l]);
			if ( ((l+1) % CELL_BYTE_WIDTH) == 0 ) {
				printf("\n");
			}
		}
		printf("\n");
	}
	printf("\n");
#endif

	printf("%s:%d: Decoding/Reconstructing blocks RS[%d:%d] cell_length=%d, using %s\n",
		 __FILE__, __LINE__, RS_K, RS_P, cell_length, (decode_isal) ? "ISA-L" : "HLS core");

	int num_vectors_per_erasure_pattern = compute_num_vectors_per_erasure_pattern (RS_K, RS_P);

	// Loop over all possible RS_K:RS_P permutations
	for ( unsigned int permutation_index = 0; permutation_index < max_permutations; permutation_index++ ) {
		uint8_t* erasure_list = (uint8_t*)(permutations[ permutation_index * NUM_ERASURES ]);

		for ( unsigned int survival_index = 0; survival_index < num_vectors_per_erasure_pattern; survival_index++ ) {
			printf("%s:%d: Reconstructing cell %d [%d/%lu] with survival pattern [%d/%d]\n",
				__FILE__, __LINE__, permutation_index, permutation_index+1, max_permutations,
					survival_index+1, num_vectors_per_erasure_pattern);

			// Decode with ISA-L
			if ( decode_isal ) {
				// NOTE: There is no need to re-compute matrices
				// We can just use decode_matrix and decode_index from hls roms generated with ISA-L
				// We are interested only in the reconstruction time, not including decdoding matrix generation
				// Find a decode matrix to regenerate all erasures from remaining frags
				// ret_val = gf_gen_decode_matrix_simple(encode_matrix, decode_matrix,
				// 				invert_matrix, temp_matrix, decode_index,
				// 				erasure_list, NUM_ERASURES, RS_K, RS_M);
				// if ( ret_val != 0 ) {
				// 	printf("%s:%d: Fail on generating decode matrix\n", __FILE__, __LINE__);
				// 	return -1;
				// }
			#ifdef DEBUG	
				print_matrix_2d(stdout, NUM_ERASURES, RS_K, (uint8_t*)(decode_matrix_rom[permutation_index]), "decode_matrix_rom ");
				print_matrix_2d(stdout, 1, RS_K, (uint8_t*)decode_index[permutation_index][survival_index], "decode_index[permutation_index]");
			#endif

				// Pack recovery array pointers as list of valid fragments
				for ( unsigned int i = 0; i < RS_K; i++ ){
					recover_srcs[i] = frag_ptrs[decode_index[permutation_index][survival_index][i]];
				}
				// Recover data
				uint8_t* decode_matrix = (uint8_t*)decode_matrix_rom[permutation_index*num_vectors_per_erasure_pattern + survival_index];
				ec_init_tables(RS_K, NUM_ERASURES, decode_matrix, g_tbls);
				ec_encode_data(cell_length, RS_K, NUM_ERASURES, g_tbls, (unsigned char **)recover_srcs, (unsigned char **)recover_outp);
				
			#ifdef DEBUG			
				printf("%s:%d: reconstructed_blocks_out:\n", __FILE__, __LINE__);
				for ( unsigned int i = 0; i < NUM_ERASURES; i++ ) {
					for ( int l = 0; l < cell_length; l++ ) {
						printf("%02x ", recover_outp[i][l]);
						if ( ((l+1) % CELL_BYTE_WIDTH) == 0 ) {
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
				for ( unsigned int i = 0; i < RS_K; i++ ) {
					for ( int l = 0; l < cell_length; l++ ) {
						((uint8_t(*)[cell_length])rs_erasure_input)[i][l] = frag_ptrs[decode_index[permutation_index][survival_index][i]][l];
					}
				}

			#ifdef DEBUG
				printf("%s:%d: rs_erasure_input:\n", __FILE__, __LINE__);
				for ( unsigned int i = 0; i < RS_K; i++ ) {
					for ( unsigned int l = 0; l < cell_length; l++ ) {
						printf("%02x ", ((uint8_t(*)[cell_length])rs_erasure_input)[i][l]);
						if ( ((l+1) % CELL_BYTE_WIDTH) == 0 ) {
							printf("\n");
						}
					}
					printf("\n");
				}
				printf("\n");
			#endif

				// Write input
				rs_erasure_csr.erasure_pattern	= permutations_pattern[permutation_index];
				rs_erasure_csr.survived_cells	= decode_index_bitstring[permutation_index][survival_index];

				// Call to kernel
				RunKernel (
						// selector,
						cell_length,
						rs_erasure_input,
						reconstructed_blocks_out,
						rs_erasure_csr
					);

			#ifdef DEBUG			
				printf("%s:%d: reconstructed_blocks_out:\n", __FILE__, __LINE__);
				for ( unsigned int i = 0; i < NUM_ERASURES; i++ ) {
					for ( int l = 0; l < cell_length; l++ ) {
						printf("%02x ", ((uint8_t(*)[cell_length])reconstructed_blocks_out)[i][l]);
						if ( ((l+1) % CELL_BYTE_WIDTH) == 0 ) {
							printf("\n");
						}
					}
				}
				printf("\n");
			#endif

				// Read data
				for ( unsigned int i = 0; i < RS_P; i++ ) {
					for ( int l = 0; l < cell_length; l++ ) {
						recover_outp[i][l] = ((uint8_t(*)[cell_length])reconstructed_blocks_out)[i][l];
					}
				}
			} // !decode_isal

			// Check that recovered buffers are the same as original
			for ( unsigned int i = 0; i < NUM_ERASURES; i++ ) {
				ret_val = memcmp(recover_outp[i], frag_ptrs[erasure_list[i]], cell_length);
				if ( ret_val ) {
					printf("%s:%d: Fail erasure recovery %d, frag %d\n", __FILE__, __LINE__, i, erasure_list[i]);
					
				#ifdef DEBUG			
					printf("%s:%d: expected:\n", __FILE__, __LINE__);
					for ( unsigned int i = 0; i < NUM_ERASURES; i++ ) {
						for ( int l = 0; l < cell_length; l++ ) {
							printf("%02x ", frag_ptrs[erasure_list[i]][l]);
							if ( ((l+1) % CELL_BYTE_WIDTH) == 0 ) {
								printf("\n");
							}
						}
					}
					printf("\n");
				#endif
						return -1;
					}
			}
		}  // survival_index over num_vectors_per_erasure_pattern
	} // permutation_index over max_permutations

	printf("%s:%d: Test passed\n RS[%d:%d]\n cell_length=%d,\n encodind with %s,\n decoding with %s,\n PRNG seed=%u\n",
		 __FILE__, __LINE__, RS_K, RS_P, cell_length,
		 (encode_isal) ? "ISA-L" : "HLS core",
		 (decode_isal) ? "ISA-L" : "HLS core",
		 prng_seed
		 );

	return ret_val;
}

void RunKernel(
		// sycl::device_selector selector,
		unsigned int cell_length,
		line_t* rs_erasure_input,
		line_t* reconstructed_blocks_out,
		rs_erasure_csr_t rs_erasure_csr
	){

	// rs_erasure (
    //              rs_erasure_input,
    //              reconstructed_blocks_out,
    //              rs_erasure_csr
    //             );
	// return;

	try {

		// Create a queue bound to the chosen device.
		// If the device is unavailable, a SYCL runtime exception is thrown.
		queue q(selector, fpga_tools::exception_handler);

		auto device = q.get_device();

		std::cout << "Running on device: "
							<< device.get_info<sycl::info::device::name>().c_str()
							<< std::endl;

		// For Functor
		// Create the device buffers
		// buffer device_read (vec_a);
		// buffer device_write(vec_b);
		// RunKernelFunctor(q, device_read, device_write, rs_erasure_csr);
		
		// For Lambda (Single mem interface)
		device_read_t  device_read  = sycl::malloc_shared<line_t>( RS_INPUT_SIZE(cell_length) , q);
		device_write_t device_write = sycl::malloc_shared<line_t>( RS_OUTPUT_SIZE(cell_length), q);

		// Check pointers are valid
		assert(device_read);
		assert(device_write);

		// Copy data in input region
		for ( unsigned int i = 0; i < RS_INPUT_SIZE(cell_length)/sizeof(line_t); i++ ) {
			// ((uint8_t*)device_read)[i] = ((uint8_t*)rs_erasure_input)[i];
			device_read[i] = rs_erasure_input[i];
		}

		// Run kernel
		RunKernelLambda(
						q,
						device_read,
						device_write,
						rs_erasure_csr
					);
		
		// Read back data
		for ( unsigned int i = 0; i < RS_OUTPUT_SIZE(cell_length)/sizeof(line_t); i++ ) {
			reconstructed_blocks_out[i] = device_write[i];
		}

	} catch (exception const &e) {
		// Catches exceptions in the host code
		std::cerr << "Caught a SYCL host exception:\n" << e.what() << "\n";

		// Most likely the runtime couldn't find FPGA hardware!
		if (e.code().value() == CL_DEVICE_NOT_FOUND) {
			std::cerr << "CL_DEVICE_NOT_FOUND\n";
		}
		std::terminate();

	} // try/catch

} // RunKernel
