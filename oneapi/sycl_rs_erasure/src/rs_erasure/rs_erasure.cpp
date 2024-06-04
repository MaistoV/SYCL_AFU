
#include <sycl/ext/intel/fpga_extensions.hpp>
// SYCL-related definitons
#include "rs_erasure_sycl.hpp"

// Forward declare the kernel names in the global scope. This FPGA best practice
// reduces compiler name mangling in the optimization reports.
class RSErasureID;

// Lambda
void RunKernelLambda( sycl::queue& q,
				int measure_latency,
				FILE* fd_latency,
				unsigned int num_erasures,
                device_read_t device_read,
                device_write_t device_write,
                rs_erasure_csr_t rs_erasure_csr
              ){

// If building in NO_SYCL mode, this function is a null macro
#ifndef NO_SYCL

// If targeting OneAPI BSP/ASP
#ifdef IS_BSP
    // make sure the device supports USM host allocations
	#ifdef IS_USM
		auto device = q.get_device();
		if (!device.get_info<sycl::info::device::usm_host_allocations>()) {
		std::cerr << "ERROR: The selected device does not support USM host allocations\n";
		exit(1);
		}
	#endif // ! IS_USM

	// Derive cell length forom CSR input
	uint64_t cell_length = rs_erasure_csr.cell_length_byte_width * LINE_BYTE_WIDTH;
	
// If using zero-copy data transfer design pattern
#ifdef ASP_ZERO_COPY
    // Input and output data for the zero-copy version
    // malloc_host allocates memory specifically in the host's address space
    line_t* in_zero_copy  = sycl::malloc_host<line_t>(RS_INPUT_SIZE(cell_length), q.get_context());
    line_t* out_zero_copy = sycl::malloc_host<line_t>(RS_OUTPUT_SIZE(cell_length, num_erasures), q.get_context());

	// Check pointers are valid
	assert ( in_zero_copy  );
	assert ( out_zero_copy );

	// Manually copy data from argument buffers to local ones
	for ( unsigned int i = 0; i < RS_INPUT_SIZE(cell_length); i++ ) {
		((uint8_t*)in_zero_copy)[i] = ((uint8_t*)device_read)[i];
	}
#else // ! ASP_ZERO_COPY
	// malloc in USM
	line_t* device_read_usm  = sycl::malloc_shared<line_t>( RS_INPUT_SIZE(cell_length) , q);
	line_t* device_write_usm = sycl::malloc_shared<line_t>( RS_OUTPUT_SIZE(cell_length, num_erasures), q);
	
	// Manually copy data from argument buffers to local ones
	for ( unsigned int i = 0; i < RS_INPUT_SIZE(cell_length); i++ ) {
		((uint8_t*)device_read_usm)[i] = ((uint8_t*)device_read)[i];
	}
#endif // ! ASP_ZERO_COPY

#endif // IS_BSP

	// Start measure by macro
	std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds> start, end;
	double time_sec = 0.0;
	if ( measure_latency ) {
		MEASURE_LATENCY_START(start);
	}

    // submit the kernel
    q.submit([&](sycl::handler &h) {
		// Kernel tags:
		// * kernel_args_restrict to specify that pointers do not alias.
		// * scheduler_target_fmax_mhz to increas fMax
		// * max_global_work_dim(0) to simplify scheduling logic
		h.single_task<RSErasureID>([=]() 
										[[intel::kernel_args_restrict]] 
										[[intel::max_global_work_dim(0)]]
										[[intel::scheduler_target_fmax_mhz(SCYL_FREQ_MHZ)]]
									 {

#ifdef IS_BSP
#ifdef ASP_ZERO_COPY
			// using a host_ptr tells the compiler that this pointer lives in the
			// hosts address space
			sycl::host_ptr<line_t> host_in_data(in_zero_copy);
			sycl::host_ptr<line_t> host_out_data(out_zero_copy);

			// Compute
			rs_erasure(
					host_in_data,
					host_out_data,
					rs_erasure_csr
				);
#else // ! ASP_ZERO_COPY
			// Launch with USM pointers
			rs_erasure(
					device_read_usm,
					device_write_usm,
					rs_erasure_csr
				);
#endif // ! ASP_ZERO_COPY
#else // ! IS_BSP
			rs_erasure(
					device_read,
					device_write,
					rs_erasure_csr
				);
#endif // ! IS_BSP
      	});
    })
	.wait();

	// End measure by macro
	if ( measure_latency ) {
		MEASURE_LATENCY_END_AND_PRINT(start, time_sec, fd_latency);
	}

#if defined(IS_BSP)
	// Copy back data from local buffer to caller's
#if defined(ASP_ZERO_COPY)
	// Copy back data from local buffer to caller's
	for ( unsigned int i = 0; i < RS_OUTPUT_SIZE(cell_length, num_erasures); i++ ) {
		((uint8_t*)device_write)[i] = ((uint8_t*)out_zero_copy)[i];
	}
	
	// Free allocations
    sycl::free(in_zero_copy, q);
    sycl::free(out_zero_copy, q);
#else // ! defined(ASP_ZERO_COPY)
	for ( unsigned int i = 0; i < RS_OUTPUT_SIZE(cell_length, num_erasures); i++ ) {
		((uint8_t*)device_write)[i] = ((uint8_t*)device_write_usm)[i];
	}
	
	// Free allocations
    sycl::free(device_read_usm, q);
    sycl::free(device_write_usm, q);
#endif // ! defined(ASP_ZERO_COPY)

#endif // defined(IS_BSP)

#endif // ! NO_SYCL

}

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

///////////////////////
// Utility functions //
///////////////////////

// Count the number of high bits in pattern
uint8_t popcount ( uint16_t pattern ) {
	uint16 pattern_acint = pattern;
	int count = 0;
	#pragma unroll
	for ( int bit_index = 0; bit_index < 16; bit_index++ ) {
		if ( pattern_acint[bit_index] ) {
			count++;
		}
	}
	return count;
}

// Return 1 if the input pattern is n-hot
int is_n_hot ( int n, uint16_t pattern ) {
	return ( popcount( pattern ) == n );
}

// Function implementing the Reed-Solomon logic
void rs_erasure (
                device_read_t  device_read,
                device_write_t device_write,
                rs_erasure_csr_t rs_erasure_csr
                ){
#ifdef NO_SYCL
	// printf("%s:%d: cell_length_byte_width	: 0x%02x\n"	, __FILE__, __LINE__, rs_erasure_csr.cell_length_byte_width	);
    printf("%s:%d: erasure_pattern			: 0x%04x\n"	, __FILE__, __LINE__, rs_erasure_csr.erasure_pattern		);
    printf("%s:%d: survived_cells			: 0x%04x\n"	, __FILE__, __LINE__, rs_erasure_csr.survived_cells			);
#endif // NO_SYCL

	/////////////////////////////////////////
	// Input read
	/////////////////////////////////////////
	// Unpack rs_erasure_csr metadata
    uint16_t    erasure_pattern	= rs_erasure_csr.erasure_pattern;
    uint16_t    survived_cells	= rs_erasure_csr.survived_cells ;
	uint32_t    cell_length		= rs_erasure_csr.cell_length_byte_width << LOG2_LINE_BYTE_WIDTH ;

	// Number of erasures (<= P)
	uint8_t		num_erasures	= popcount(erasure_pattern);

// Debug device_read
#ifdef NO_SYCL
	printf("%s:%d: num_erasures: %u\n", __FILE__, __LINE__, num_erasures);
	printf("%s:%d: device_read:\n", __FILE__, __LINE__);
	for ( unsigned int i = 0; i < (cell_length * RS_K); i++ ) {
		printf("%02x ", ((uint8_t*)device_read)[i]);
		if ( ((i+1) % LINE_BYTE_WIDTH) == 0 ) {
			printf("\n");
		}
	}
	printf("\n");
#endif // NO_SYCL

	// Check input values
#if defined(FPGA_EMULATOR) || defined(NO_SYCL)
	assert( ( num_erasures <= MAX_ERASURES )	); // Maximum supported erasures
	assert( is_n_hot ( RS_K, survived_cells & RS_PATTERN_MASK ) );
	assert( cell_length >= CELL_LENGTH_MIN 		 );
	assert( (cell_length % LINE_BYTE_WIDTH) == 0 ); // Must be an integer multiple
#endif

LOOP_LINES:
	// Loop over interface lines in a cell
	#define NUM_LINES (cell_length / LINE_BYTE_WIDTH)
	#pragma unroll 1 // Explicit no unroll
	for ( unsigned int line_index = 0; line_index < NUM_LINES; line_index++ ) {
		// Array of k survived cell lines, force it as registers
		[[intel::fpga_register]] line_t survived_cell_lines [RS_K];

	LOOP_READ_LINES:
		// Read RS_K lines for each input cell
		// Strided memory read
		#pragma unroll LOOP_READ_CELLS_UNROLL
		for ( unsigned int cell_index = 0; cell_index < RS_K; cell_index++ ){
			survived_cell_lines[cell_index] = device_read[ (cell_index * NUM_LINES) + line_index ];
		}
		// Debug survived_cell_lines
		#ifdef NO_SYCL
			for ( unsigned int cell_index = 0; cell_index < RS_K; cell_index++ ){
				printf("%s:%d: survived_cell_lines[%d] for line_index=%d:\n", __FILE__, __LINE__, cell_index, line_index);
				for ( int byte_index = 0; byte_index < LINE_BYTE_WIDTH; byte_index++ ) {
					printf("%02x ", ((uint8_t (*)[LINE_BYTE_WIDTH])survived_cell_lines)[cell_index][byte_index] );
				}
				printf("\n");
			}
			printf("\n");
		#endif // NO_SYCL

		uint8_t recontruction_counter = 0;
		LOOP_ERASURES:
			#pragma unroll 1 // Explicit no unroll
			// uint8_t since we are assuming RS_M <= 16
			for ( uint8_t erasure_pattern_bit_index = 0; erasure_pattern_bit_index < RS_M; erasure_pattern_bit_index++ ) {
				
				// If we get a high bit
				uint16 erasure_pattern_uint16 = erasure_pattern;
				if ( erasure_pattern_uint16[erasure_pattern_bit_index] ) {

					////////////////
					// ROM lookup //
					////////////////

					// Extract the one-hot erasure patterns for the next erasure
					#define ERASURE_ONEHOT_MASK (erasure_pattern & (0x1u << erasure_pattern_bit_index))
					// ROM index of reconstruction vector
					uint16_t vector_index = rs_rom_lookup( ERASURE_ONEHOT_MASK, survived_cells );

					// Schratchpad memory buffering ROM data
					// If necessary, force it as register [[intel::fpga_register]]
					uint8_t reconstruction_vector	[SCRATCHPAD_DEPTH];

				LOOP_ROM_LOOKUP:
					// Read decoding matrix from the right ROM address
					#pragma unroll
					for ( unsigned int j = 0; j < RS_K; j++ ) {
						reconstruction_vector[j] = decode_matrix_rom[vector_index][j];
					}

				// Debug reconstruction_vector
				#ifdef NO_SYCL
					printf("%s:%d: vector_index=%hu\n", __FILE__, __LINE__, vector_index);
					printf("%s:%d: reconstruction_vector: ", __FILE__, __LINE__ );
					for ( unsigned int j = 0; j < RS_K; j++ ) {
						printf("%hhu ", reconstruction_vector[j]);
					}
					printf("\n");
				#endif // NO_SYCL

				/////////////////
				// GF multiply //
				/////////////////
				// Reset all the recontructing bits
				line_t reconstructed_cell_line;
				reconstructed_cell_line = (line_t)0u;

				// Loop over bytes in a cell
			LOOP_BYTES:
				#pragma unroll // full unroll
				for ( unsigned int cell_byte_index = 0; cell_byte_index < LINE_BYTE_WIDTH; cell_byte_index++ ) {
		LOOP_READ_SCHRATCHPAD:
					// Loop over bytes in scratchpad
					#pragma unroll // full unroll
					for ( uint8_t reconstruction_vector_byte_index = 0; reconstruction_vector_byte_index < RS_K; reconstruction_vector_byte_index++ ) {
						// Read from Scratchpad Memory
						// NOTE: this is a contiguous access pattern, let the compiler coalesce it
						uint8 tmp_byte = reconstruction_vector[reconstruction_vector_byte_index];

						uint8_t reconstructed_byte = (uint8_t)0u;
		LOOP_BITS:
						// Loop over bits in each byte
						#pragma unroll // full unroll
						for ( uint8_t bit_index = 0; bit_index < GF_ORDER; bit_index++ ) {
								// Perform (AND or mux) multiplication and (XOR) accumulation (addition in GF(2^8))
								uint8 survived_byte = ((uint8 (*)[LINE_BYTE_WIDTH])survived_cell_lines)[reconstruction_vector_byte_index][cell_byte_index];
								reconstructed_byte ^= ( survived_byte[bit_index] ) // Multiplication
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

				////////////////////////////
				// Write out to interface //
				////////////////////////////
				unsigned int write_line = line_index + (recontruction_counter * NUM_LINES);
				device_write[ write_line ] = reconstructed_cell_line;
			
			// Debug reconstructed_cell_line
			#ifdef NO_SYCL
				printf("%s:%d: recontruction_counter %d: \n", __FILE__, __LINE__, recontruction_counter );
				printf("%s:%d: Output data on line_index %d @%016x: \n", __FILE__, __LINE__, line_index, write_line );
				for ( unsigned int byte_index = 0; byte_index < sizeof(reconstructed_cell_line); byte_index++ ) {
					printf("%02x ", ((uint8_t*)&reconstructed_cell_line)[byte_index]);
				}
				printf("\n\n");
			#endif // NO_SYCL

				// Increment counter
				recontruction_counter++;

			} // erasure_pattern_uint16[erasure_pattern_bit_index]
		} // erasure_pattern_bit_index
	} // line_index

} // rs_erasure

