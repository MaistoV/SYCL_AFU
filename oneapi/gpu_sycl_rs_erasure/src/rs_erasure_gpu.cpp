// rs_erasure_gpu.cpp
// #include <sycl/ext/intel/fpga_extensions.hpp>
// SYCL-related definitons
#include "rs_erasure_sycl.hpp"


///////////////////////
// Utility functions //
///////////////////////

// Count the number of high bits in pattern
uint8_t popcount ( uint16_t pattern ) {
	uint16 pattern_acint = pattern;
	int count = 0;
	#pragma unroll
	for ( int bit_index = 0; bit_index < 16; bit_index++ ) {
        // Check LSB bit
        if ( pattern_acint & 0x1 ) {
            // Increment counter
            count++;
        }
        // Shift down
        pattern_acint >>= 1;
	}
	return count;
}

// Return 1 if the input pattern is n-hot
int is_n_hot ( int n, uint16_t pattern ) {
	return ( popcount( pattern ) == n );
}

// Mux ROM values among RS codes
// #ifdef RS_3_2
// #define MAX_PATTERNS 20
// 	#include "rs_rom_lookup_3_2.c"
// 	#define rs_rom_lookup rs_3_2_rom_lookup
// #endif
// #ifdef RS_6_3
// #define MAX_PATTERNS 252
// 	#include "rs_rom_lookup_6_3.c"
// 	#define rs_rom_lookup rs_6_3_rom_lookup
// #endif
// Max for 3:2
// MOCK: assumig a simple function to get the wright vector index
#define rs_rom_lookup(erasure_pattern, survival_pattern) (erasure_pattern_bit_index)
    // (erasure_pattern ^ survival_pattern ^ GF_POLY)



// Forward declare the kernel names in the global scope. This FPGA best practice
// reduces compiler name mangling in the optimization reports.
class RSErasureID;

// Lambda
void RunKernelLambda( sycl::queue& q,
				int measure_latency,
				FILE* fd_latency,
				unsigned int num_erasures,
                device_read_t host_read,
                device_write_t host_write,
                rs_erasure_csr_t rs_erasure_csr
              ){

	
	// Start measure by macro
	std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds> start, end;
	double time_sec = 0.0;
	// if ( measure_latency ) {
	// 	MEASURE_LATENCY_START(start);
	// }

    //////////////////
    // Prepare data //
    //////////////////
    // Use device memory
    const uint32_t   cell_length_ = rs_erasure_csr.cell_length_byte_width << LOG2_LINE_BYTE_WIDTH ;
    const auto    device_read  = sycl::malloc_device<line_t>(RS_INPUT_SIZE(cell_length_), q);
    const auto    device_csr   = sycl::malloc_device<rs_erasure_csr_t>(sizeof(rs_erasure_csr_t), q);
    auto          device_write = sycl::malloc_device<line_t>(RS_OUTPUT_SIZE(cell_length_, num_erasures), q);
    
    // DMA ROMS to device memory
    q.memcpy(device_read, host_read, RS_INPUT_SIZE(cell_length_));
    // DMA input CSR to device memory
    q.memcpy(device_csr, &rs_erasure_csr, sizeof(rs_erasure_csr_t));
    // Wait for DMA to finish
    q.wait();
    
	if ( measure_latency ) {
		MEASURE_LATENCY_START(start);
	}

    // submit the kernel
    q.submit([&](sycl::handler &h) {
		// Kernel tags:
		// * kernel_args_restrict to specify that pointers do not alias.
		// h.single_task<RSErasureID>([=]() 
		h.single_task([=]() 
										[[intel::kernel_args_restrict]] 
										[[intel::max_global_work_dim(0)]]
									 {
            //////////////////
            // Prepare data //
            //////////////////
                                         
            /////////////////
            // Call kenrel //
            /////////////////
        	// Unpack rs_erasure_csr metadata
            const uint16_t    erasure_pattern	= device_csr->erasure_pattern;
            const uint16_t    survived_cells	= device_csr->survived_cells ;
        	const uint32_t    cell_length		= device_csr->cell_length_byte_width << LOG2_LINE_BYTE_WIDTH ;
        	
            #define NUM_LINES (device_csr->cell_length_byte_width)	
LOOP_LINES: // Loop over lines in a cell
            #pragma unroll
        	for ( auto line_index = 0; line_index < NUM_LINES; line_index++ ) {
        		// Array of k survived cell lines, force it as registers
                line_t survived_cell_lines [RS_K];
        		
LOOP_READ_LINES:// Read RS_K lines for each input cell
        		// Strided memory read
        		#pragma unroll
        		for ( auto cell_index = 0; cell_index < RS_K; cell_index++ ){
        			survived_cell_lines[cell_index] = device_read[ (cell_index * NUM_LINES) + line_index ];
        		}

                // Count reconstructions
                // - uint8_t since we are assuming RS_M <= 16
        		uint8_t recontruction_counter = 0;
LOOP_ERASURES:  // Loop over erasure patern bits
                #pragma unroll
        		for ( auto erasure_pattern_bit_index = 0; erasure_pattern_bit_index < RS_M; erasure_pattern_bit_index++ ) {
        			// If we get a high bit
                    uint16_t mask = erasure_pattern & (1 << erasure_pattern_bit_index);
        			if ( mask ) {
        				////////////////
        				// ROM lookup //
        				////////////////
        				// Schratchpad memory buffering ROM data
        				uint8_t reconstruction_vector	[SCRATCHPAD_DEPTH];
                        {
            				// Extract the one-hot erasure patterns for the next erasure
            				#define ERASURE_ONEHOT_MASK (erasure_pattern & (0x1u << erasure_pattern_bit_index))
            				// ROM index of reconstruction vector
            				uint16_t vector_index = rs_rom_lookup( ERASURE_ONEHOT_MASK, survived_cells );
                            // Include ROMS locally                        
                            #ifdef RS_3_2
                            	#include "roms/rs_rom_3_2.c"
                            #endif
                            #ifdef RS_6_3
                            	#include "roms/rs_rom_6_3.c"
                            #endif
    LOOP_ROM_LOOKUP:		// Read decoding matrix from the right ROM address
                            #pragma unroll RS_K
            				for ( auto j = 0; j < RS_K; j++ ) {
            					reconstruction_vector[j] = decode_matrix_rom[vector_index][j];
            				}
                        }
        
        				/////////////////
        				// GF multiply //
        				/////////////////
        
        				// Reset all the recontructing bits
        				line_t reconstructed_cell_line;
        				reconstructed_cell_line = (line_t)0u;
        
LOOP_BYTES:        		// Loop over bytes in a cell
			    		#pragma unroll // full unroll
        				for ( auto cell_byte_index = 0; cell_byte_index < LINE_BYTE_WIDTH; cell_byte_index++ ) {
LOOP_READ_SCHRATCHPAD:		// Loop over bytes in scratchpad
                    		#pragma unroll // full unroll
        					for ( auto reconstruction_vector_byte_index = 0; reconstruction_vector_byte_index < RS_K; reconstruction_vector_byte_index++ ) {
        						// Read from Scratchpad Memory
                                uint8_t survived_byte = ((uint8 (*)[LINE_BYTE_WIDTH])survived_cell_lines)[reconstruction_vector_byte_index][cell_byte_index];
                                // Prepare output
        						uint8_t reconstructed_byte = (uint8_t)0u;
LOOP_BITS:        		 		// Loop over bits in each byte (Russian Paesant-like algorithm)
        						#pragma unroll
         						for ( auto bit_index = 0; bit_index < GF_ORDER; bit_index++ ) {
                						// NOTE: this is a contiguous access pattern, let the compiler coalesce it
                						uint8_t reconstruction_vector_byte = reconstruction_vector[reconstruction_vector_byte_index];
                                        // Expand to 16 bits
                                        uint16_t reconstruction_vector_16 = (uint16_t)reconstruction_vector_byte;
        								// Perform (AND or mux) multiplication and (XOR) accumulation (addition in GF(2^8))
                                        if (reconstruction_vector_16 & (1 << bit_index)) {
                                            // Multiply (Left shift temp bit_index times with modular reduction)
                                            for (int j = 0; j < bit_index; ++j) {
                                                survived_byte <<= 1;
                                                // If the 9th bit is set
                                                if (survived_byte & 0x100) { survived_byte ^= GF_POLY; }
                                            }
                                            // Accumulate
                                            reconstructed_byte ^= (uint8_t)survived_byte;
                                        }
                                }
                                
        						// Accumulate new value
        						((uint8*)&reconstructed_cell_line)[cell_byte_index] ^= reconstructed_byte;
        					}
        
        				} // cell_byte_index
        
        				////////////////////////////
        				// Write out to interface //
        				////////////////////////////
        				unsigned int write_line = line_index + (recontruction_counter * NUM_LINES);
        				device_write[ write_line ] = reconstructed_cell_line;
            			
                        // Count recounstructions
        				recontruction_counter++;
        
        			} // erasure_pattern_uint16[erasure_pattern_bit_index]
        		} // erasure_pattern_bit_index
        	} // line_index
      	});
    })
	.wait();
    
	// End measure by macro
	if ( measure_latency ) {
		MEASURE_LATENCY_END_AND_PRINT(start, time_sec, fd_latency);
	}

    ////////////////////
    // Read-back data //
    ////////////////////
    // DMA outputs from device memory
    q.memcpy(host_write, device_write, RS_INPUT_SIZE(cell_length_))
        .wait();
    
	// // End measure by macro
	// if ( measure_latency ) {
	// 	MEASURE_LATENCY_END_AND_PRINT(start, time_sec, fd_latency);
	// }

    // Clean up
    sycl::free(device_read, q);
    sycl::free(device_write, q);
    sycl::free(device_csr, q);
}

