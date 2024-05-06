// This is just a wrapper file, selecting the proper host source file

#include <iostream>
#include <chrono>
#include <unistd.h> // for usleep

// OPAE APIs
#include <opae/fpga.h>

// State from the AFU's JSON file, extracted using OPAE's afu_json_mgr script
#include "afu_json_info.h"

// Register map emitted for DFL
#include "afu_regmap.h"

// Utility functions
#include "sycl_afu_utils.h"

// RS header
#include "rs_erasure.hpp"

// For verification
#include <getopt.h>
// For ISA-L
// #include <isa-l.h>
// For compute_max_erasure_patterns, gf_gen_cauchy1_matrix, compute_num_vectors_per_erasure_pattern, gf_gen_decode_matrix_simple
#include "rs_erasure/roms/src/rs_rom_utils.h"

// Measure macros for latency
#include "measure_latency.h"

// Wrap OPAE function call sequences
#include "opae_simple_wrapper.h"

// For compute_max_erasure_patterns, gf_gen_cauchy1_matrix, compute_num_vectors_per_erasure_pattern
#include "rs_erasure/roms/src/rs_rom_utils.h"

// Decode cell_length for Bytes, KBs or MBs
int decode_cell_length (
		char cell_length_byte_power[2],
		unsigned int* cell_length_byte,
		const unsigned int cell_length
		);

// Microseconds wait for AFU CSR polling
#define SLEEP_TIME_US 10000

#ifdef MULTI_ERASURE_SIMPLE
    #pragma message "[INFO] Importing sycl_rs_erasure_multi_erasure.cpp"
    #include "sycl_rs_erasure_multi_erasure.cpp"
#else // ! MULTI_ERASURE_SIMPLE
    #pragma message "[INFO] Importing sycl_rs_erasure_one_erasure.cpp"
    #include "sycl_rs_erasure_one_erasure.cpp"
#endif // ! MULTI_ERASURE_SIMPLE

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