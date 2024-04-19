#ifndef _RS_ERASURE_H_
#define _RS_ERASURE_H_

///////////////////////////////////////////////////
// This header contains all the non-SYCL defines //
///////////////////////////////////////////////////

#define GF_POLY 	29	// Generator polynomial of the Galois field: 0x1d
#define GF_ORDER	8	// Order of Galois filed (2^GF_ORDER)

#define SCRATCHPAD_WIDTH	sizeof(uint8_t)			// Total number of cells
#define SCRATCHPAD_DEPTH	(RS_K/SCRATCHPAD_WIDTH)	// Number of words of SCRATCHPAD Memory

// Match Native Avalon MM hostchan parameters
// see ofs_plat_if_top_config.vh and ofs_plat_avalon_mem_rdwr_if.sv
#define LINE_BIT_WIDTH 				512U				// Width of interface in bits
#define LINE_BYTE_WIDTH 			(LINE_BIT_WIDTH/8) 	// Width of interface in bytes
#define LOG2_LINE_BYTE_WIDTH		6u					// log2(CELL_BYTE_WIDTH) = log2(64)

// Utility macros for cell length
#define _1B						(1			 	)	// 1 Byte
#define _64KB					(64 * 1024 		)	// 64KiloBbyte
#define _1MB					(1024 * 1024 	)	// 1Megabyte
#define _2MB					(2 * 1024 * 1024)	// 2Megabytes
#define CELL_LENGTH_MAX			(_1MB) 				// With huge pages 2MB
#define CELL_LENGTH_MIN 		(LINE_BYTE_WIDTH)	// MIN (by design)
#ifndef CELL_LENGTH_DEFAULT
	// #warning CELL_LENGTH_DEFAULT undefined, defaulting to CELL_BYTE_WIDTH*2=128
	#define CELL_LENGTH_DEFAULT (CELL_LENGTH_MIN*2)	// Length of RS cell in bytes
#endif

// Utility macros for input and output buffer size
#define MAX_ERASURES 								(RS_P)								// Number of supported paraller erasures/reconstructions
#define RS_INPUT_SIZE(cell_length)					( cell_length * RS_K 			)	// Size of input buffer
#define RS_OUTPUT_SIZE(cell_length,num_erasures)	( cell_length * num_erasures 	)	// Size of output buffer

// Control and Status Register layout for rs_erasure
// NOTE: this layout requires K+P <= 16
typedef struct rs_erasure_csr {
	 uint16_t	erasure_pattern;	 	// Which cells were erased (1-hot to p-hot)
	 uint16_t	survived_cells;	  		// Which k cells of the k+p are provided for reconstruction
	 uint32_t	cell_length_byte_width;	// Cell length in multiples of CELL_BYTE_WIDTH
} rs_erasure_csr_t;

// Mux ROM values among RS codes
// TODO: this could be auto generated
#ifdef RS_3_2
	#define RS_K 3
	#define RS_P 2
	#include "roms/rs_erasure_patterns_3_2.c"
	#include "roms/rs_decode_3_2.c"
	#include "roms/rs_rom_3_2.c"
	#define RS_PATTERN_MASK 			0x001fu
	#define permutations		 		erasures_3_2
	#define erasure_patterns 			erasures_patterns_3_2
	#define decode_index     			decode_index_3_2
	#define	decode_index_bitstring		decode_index_3_2_bitstring
	#define decode_matrix_rom			DECMAT_ROM_3_2
#endif
#ifdef RS_6_3
	#define RS_K 6
	#define RS_P 3
	#include "roms/rs_erasure_patterns_6_3.c"
	#include "roms/rs_decode_6_3.c"
	#include "roms/rs_rom_6_3.c"
	#define RS_PATTERN_MASK 			0x01ffu
	#define permutations		 		erasures_6_3
	#define erasure_patterns  			erasures_patterns_6_3
	#define decode_index     			decode_index_6_3
	#define	decode_index_bitstring		decode_index_6_3_bitstring
	#define decode_matrix_rom			DECMAT_ROM_6_3
#endif
#ifdef RS_10_4
	#define RS_K 10
	#define RS_P 4
	#include "roms/rs_erasure_patterns_10_4.c"
	#include "roms/rs_decode_10_4.c"
	#include "roms/rs_rom_10_4.c"
	#define RS_PATTERN_MASK 			0x3fffu
	#define permutations		 		erasures_10_4
	#define erasure_patterns  			erasures_patterns_10_4
	#define decode_index     			decode_index_10_4
	#define	decode_index_bitstring		decode_index_10_4_bitstring
	#define decode_matrix_rom 			DECMAT_ROM_10_4
#endif

// Safety macro definition check
#ifndef RS_K
#ifndef RS_P
	#error RS_K and RS_P undefined, define [RS_3_2 | RS_6_3 | RS_10_4]
#endif
#endif

#define RS_M (RS_K + RS_P) // Total number of cells
#if RS_M > 16
	#error Current implementation does not support M = K + P > 16
#endif

#endif // _RS_ERASURE_H_