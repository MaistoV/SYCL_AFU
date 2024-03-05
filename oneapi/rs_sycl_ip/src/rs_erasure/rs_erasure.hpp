
#ifndef _RS_ERASURE_H_
#define _RS_ERASURE_H_

///////////////////////////////////////////////////
// This header contains all the non-SYCL defines //
///////////////////////////////////////////////////

#define GF_POLY 	29	// Generator polynomial of the Galois field: 0x1d
#define GF_ORDER	8	// Order of Galois filed (2^GF_ORDER)

#define SCRATCHPAD_WIDTH	sizeof(uint8_t)			// Total number of cells
#define SCRATCHPAD_DEPTH	(RS_K/SCRATCHPAD_WIDTH)	// Number of words of SCRATCHPAD Memory

#define DATA_BYTE_WIDTH 			64u // Width of interface 512 bits /8
#define LOG2_DATA_BYTE_WIDTH		6u	// log2(DATA_BYTE_WIDTH)

#define _1B						(1			 	)	// 1 Byte
#define _64KB					(64 * 1024 		)	// 64KiloBbyte
#define _1MB					(1024 * 1024 	)	// 1Megabyte
#define _2MB					(2 * 1024 * 1024)	// 2Megabytes
#define CELL_LENGTH_MAX			(_1MB) 				// With huge pages 2MB
#define CELL_LENGTH_MIN 		(DATA_BYTE_WIDTH)	// MIN (by design)
#ifndef CELL_LENGTH_DEFAULT
	// #warning CELL_LENGTH_DEFAULT undefined, defaulting to DATA_BYTE_WIDTH*2=128
	#define CELL_LENGTH_DEFAULT (DATA_BYTE_WIDTH*2)	// Length of RS cell in bytes
#endif

#define NUM_ERASURES 		1	// Number of paraller erasures to corrections
#define RS_INPUT_SIZE(cell_length)	( cell_length * RS_K 		)	// Size of input buffer
#define RS_OUTPUT_SIZE(cell_length)	( cell_length * NUM_ERASURES 	)	// Size of output buffer

// Control and Status Register layout for rs_erasure
typedef struct rs_erasure_csr {
	//  uint8_t	 code_id;				// RS schema (Constant RS6:3 for now)
	//  uint8_t	 cell_length_id;	  	// Length of each cell (Constant for now)
	 uint16_t	erasure_pattern;	 	// Which cells were erased (1-hot for now)
	 uint16_t	survived_cells;	  		// Which k cells of the k+p are provided for reconstruction
	 uint32_t	cell_length_byte_width;			// Cell length in multiples of DATA_BYTE_WIDTH
} rs_erasure_csr_t;

// Mux ROM values among RS codes
#ifdef RS_3_2
	#define RS_K 3
	#define RS_P 2
	#include "roms/rs_erasure_patterns_3_2.c"
	#include "roms/rs_decode_3_2.c"
	#include "roms/rs_rom_3_2.c"
	#define PERMUTATION_PATTERN_MASK 	0x001fu
	#define permutations		 		erasures_3_2
	#define permutations_pattern  		erasures_patterns_3_2
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
	#define PERMUTATION_PATTERN_MASK 	0x01ffu
	#define permutations		 		erasures_6_3
	#define permutations_pattern  		erasures_patterns_6_3
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
	#define PERMUTATION_PATTERN_MASK 	0x3fffu
	#define permutations		 		erasures_10_4
	#define permutations_pattern  		erasures_patterns_10_4
	#define decode_index     			decode_index_10_4
	#define	decode_index_bitstring		decode_index_10_4_bitstring
	#define decode_matrix_rom 			DECMAT_ROM_10_4
#endif

#ifndef RS_K
#ifndef RS_P
	#error RS_K and RS_P undefined, define [RS_3_2 | RS_6_3 | RS_10_4]
#endif
#endif

// #define STR(x) #x
// #define XSTR(x) STR(x)
// #pragma message "Building RS[" XSTR(RS_K) ":" XSTR(RS_P) "]"

#define RS_M (RS_K + RS_P) // Total number of cells

#endif // _RS_ERASURE_H_