
#include <sycl/ext/intel/fpga_extensions.hpp>
// SYCL-related definitons
#include "rs_erasure_sycl.hpp"

// Forward declare the kernel names in the global scope. This FPGA best practice
// reduces compiler name mangling in the optimization reports.
class RSErasureID;

// Lambda
void RunKernelLambda( sycl::queue& q, 
                line_t* master_read, 
                line_t* master_write,
                rs_erasure_csr_t rs_erasure_csr
              ){

    // submit the kernel
    q.submit([&](sycl::handler &h) {
      // Use kernel_args_restrict to specify that pointers do not alias.
      h.single_task<RSErasureID>([=]() [[intel::kernel_args_restrict]] {
        rs_erasure(master_read, master_write, rs_erasure_csr);
      });
    });
}

// Functor
// void RunKernelFunctor( sycl::queue& q, 
//                 line_t* master_read, 
//                 line_t* master_write,
//                 rs_erasure_csr_t csr
//               ){
//     // Submit the kernel
//     q.single_task( rs_erasure_functor{master_read, master_write, csr} )
//       .wait();
// }

// Mux ROM values among RS codes
#ifdef RS_3_2
	#include "roms/rs_rom_lookup_3_2.c"
	#define rs_rom_lookup rs_3_2_rom_lookup
#endif
#ifdef RS_6_3
	#include "roms/rs_rom_lookup_6_3.c"
	#define rs_rom_lookup rs_6_3_rom_lookup
#endif
#ifdef RS_10_4
	#include "roms/rs_rom_lookup_10_4.c"
	#define rs_rom_lookup rs_10_4_rom_lookup
#endif

// Undefine debug macros for latency in reports
#ifndef FPGA_EMULATOR
	#undef DEBUG
	#undef DEBUG0
#endif

// Utility function
// Return 1 if the input pattern is n-hot
int is_n_hot ( int n, uint16_t pattern ) {
	uint16 pattern_acint = pattern;
	int count = 0;
	for ( int bit_index = 0; bit_index < 16; bit_index++ ) {
		if ( pattern_acint[bit_index] ) {
			count++;
		}
	}
#ifdef DEBUG
	// printf("%s:%d: count=%d\n", __FILE__, __LINE__, count );
#endif
	return ( count == n );
}

// Function implementing the Reed-Solomon logic
void rs_erasure (
                line_t* master_read,
                line_t* master_write,
                rs_erasure_csr_t rs_erasure_csr 
                ){
#ifdef DEBUG0
    // // printf("%s:%d: code_id		: 0x%02x\n"		, __FILE__, __LINE__, rs_erasure_csr.code_id			);
	// printf("%s:%d: cell_length_BYTE_WIDTH		: 0x%02x\n"	, __FILE__, __LINE__, rs_erasure_csr.cell_length_BYTE_WIDTH		);
    // printf("%s:%d: erasure_pattern	: 0x%04x\n"	, __FILE__, __LINE__, rs_erasure_csr.erasure_pattern	);
    // printf("%s:%d: survived_cells	: 0x%04x\n"	, __FILE__, __LINE__, rs_erasure_csr.survived_cells	);
#endif

	/////////////////////////////////////////
	// Input read 
	/////////////////////////////////////////
	// Unpack rs_erasure_csr metadata
    // uint8_t     code_id			= rs_erasure_csr.code_id		 ;
    uint16_t    erasure_pattern	= rs_erasure_csr.erasure_pattern;
    uint16_t    survived_cells	= rs_erasure_csr.survived_cells ;
	uint32_t    cell_length		= rs_erasure_csr.cell_length_BYTE_WIDTH << BYTE_WIDTH_BITS ;
	
#ifdef DEBUG
	// printf("%s:%d: master_read:\n", __FILE__, __LINE__);
	for ( unsigned int i = 0; i < cell_length*RS_K; i++ ) {
		// printf("%02x ", ((uint8_t*)master_read)[i]);
		if ( ((i+1) % BYTE_WIDTH) == 0 ) {
			// printf("\n");
		}
	}
	// printf("\n");
#endif

	// Subset of posisble values for now
#ifdef FPGA_EMULATOR
	assert( is_n_hot	( 1, erasure_pattern )	);
	assert( is_n_hot 	( RS_K, survived_cells & PERMUTATION_PATTERN_MASK )	);
	assert( cell_length >= CELL_LENGTH_MIN 		); 
	assert( (cell_length % BYTE_WIDTH) == 0 ); // Must be an integer multiple
#endif
	// Schratchpad memory buffering ROM data
	uint8_t scratchpad_register	[SCRATCHPAD_DEPTH];	 
	
	// ROM address
	uint16_t decmat_idx = rs_rom_lookup( erasure_pattern, survived_cells ); 

LOOP_WRITE_SCHRATCHPAD:
	// Read decoding matrix from the right ROM address
	#pragma unroll
	for ( unsigned int j = 0; j < RS_K; j++ ) {
		scratchpad_register[j] = decode_matrix_rom[decmat_idx][j]; 
	}

#ifdef DEBUG
	// printf("%s:%d: decmat_idx=%hu\n", __FILE__, __LINE__, decmat_idx);
	// printf("%s:%d: scratchpad_register: ", __FILE__, __LINE__ );
	for ( unsigned int j = 0; j < RS_K; j++ ) {
		// printf("%hhu ", scratchpad_register[j]);
	}
	// printf("\n");
#endif

	/////////////////////////////////////////
	// Logic from mat_mult_gf 
	/////////////////////////////////////////
LOOP_lineS:
	// Loop over CCI lines in a cell
	#define NUM_lineS	(cell_length / BYTE_WIDTH)
	for ( unsigned int line_index = 0; line_index < NUM_lineS; line_index++ ) {
		// Array of k survived cell lines
		line_t survived_cell_lines[RS_K];
		
LOOP_CELLS:
		// Read a single CCI line for each input cell
		// Strided memory read
		#pragma unroll
		for ( unsigned int cell_index = 0; cell_index < RS_K; cell_index++ ){
			survived_cell_lines[cell_index] = master_read[ (cell_index * NUM_lineS) + line_index ];
		}	

	#ifdef DEBUG
		for ( unsigned int cell_index = 0; cell_index < RS_K; cell_index++ ){
			// printf("%s:%d: survived_cell_lines[%d] for line_index=%d:\n", __FILE__, __LINE__, cell_index, line_index);
			for ( int byte_index = 0; byte_index < BYTE_WIDTH; byte_index++ ) {
				// printf("%02x ", ((uint8_t (*)[BYTE_WIDTH])survived_cell_lines)[cell_index][byte_index] );
			}
			// printf("\n");
		}
		// printf("\n");
	#endif

		// Reset all the parity bits
		line_t reconstructed_cell_line;
		// #pragma unroll
		reconstructed_cell_line = (line_t)0u;

		// Loop over bytes in a cell
LOOP_BYTES:
		#pragma unroll
		for ( unsigned int cell_byte_index = 0; cell_byte_index < BYTE_WIDTH; cell_byte_index++ ) {
LOOP_READ_SCHRATCHPAD:
			// Loop over bytes in schratchpad
			// NOTE: This loop is serial but shows some pipelining parallelism
			#pragma unroll
			for ( uint8_t scratchpad_register_byte_idx = 0; scratchpad_register_byte_idx < RS_K; scratchpad_register_byte_idx++ ) {
				// Read from Scratchpad Memory
				// NOTE: this is a contiguous access pattern, let the compiler coalesce it
				uint8 tmp_byte = scratchpad_register[scratchpad_register_byte_idx];

				uint8_t reconstructed_byte = (uint8_t)0u;
LOOP_BITS:
				// loop for all the bits of the input data
				#pragma unroll
				for ( uint8_t bit_idx = 0; bit_idx < GF_ORDER; bit_idx++ ) {
						// Perform (AND or mux) multiplication and (XOR) accumulation (addition in GF(2^8))
						uint8 survived_byte = ((uint8 (*)[BYTE_WIDTH])survived_cell_lines)[scratchpad_register_byte_idx][cell_byte_index]; 
						reconstructed_byte ^= ( survived_byte[bit_idx] ) // Multiplication 
																		? tmp_byte 		// +1
																		: (uint8)0u;	// +0

						// Save MSB before shifting it out	
						uint1 msb_tmp_byte = tmp_byte[7];
						// Multiply in GF(2^8) (shift and reduce)
						tmp_byte <<= 1u;
						if ( msb_tmp_byte ){
							tmp_byte = tmp_byte ^ GF_POLY;
						}
				}

				// Accumulate new value
				((uint8*)&reconstructed_cell_line)[cell_byte_index] ^= reconstructed_byte;
			}

		} // cell_byte_index

		/////////////////////////////////////////
		// Write out to CCI-P master
		/////////////////////////////////////////
		master_write[ line_index ] = reconstructed_cell_line;

	#ifdef DEBUG
		// printf("%s:%d: Output data on line_index %d: \n", __FILE__, __LINE__, line_index );
		for ( unsigned int byte_index = 0; byte_index < sizeof(reconstructed_cell_line); byte_index++ ) {
			// printf("%02x ", ((uint8_t*)&reconstructed_cell_line)[byte_index]);
		}
		// printf("\n\n");
	#endif

	} // line_index
}		

