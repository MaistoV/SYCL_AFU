/**********************************************************************
  Copyright(c) 2011-2018 Intel Corporation All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of Intel Corporation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <isa-l.h>

// TODO: Build this as an obj file in production
#ifdef BUILDING
#define MMAX 255
#define KMAX 255
#include <stdint.h>
#include "rs_rom_utils.c"
#else
#include "rs_rom_utils.h"
#endif

int usage( char** argv ) {
	fprintf(stderr,
		"Usage: %s [options]\n"
		"  -h      	This help\n"
		"  -k <val>	Number of source fragments\n"
		"  -p <val>	Number of parity fragments\n"
		"  -o 		Output directory\n",
		argv[0]
	);
	exit(0);
}

int main(int argc, char *argv[]) {
	FILE *fd_erasure_patterns, *fd_pattern;
	char filedir[256] = "./";
	char tmp_string[256];
	
	int ret_val = 0;
	// Default params
	unsigned int rs_k = 6, rs_p = 3, len = 1;	
	int use_cauchy_matrix = 1;

	// Fragment buffer pointers
	uint8_t* bitstring;

	int c;
	while ( ( c = getopt(argc, argv, "k:p:o:h") ) != -1 ) {
		switch (c) {
		case 'k':
			rs_k = atoi(optarg);
			break;
		case 'p':
			rs_p = atoi(optarg);
			break;
		case 'c':
			use_cauchy_matrix = atoi(optarg);
			break;
		case 'o':
			strcpy(filedir, optarg);
			break;
		case 'h':
		default:
			usage ( argv );
			break;
		}
	}

	unsigned int rs_m = rs_k + rs_p;	// Total number of cells
	unsigned int num_erasures = rs_p; // Number of erasures


	// Check for valid parameters
	if ( rs_m > MMAX || rs_k > KMAX || rs_p < 1 || rs_k < 1 || num_erasures > rs_p ) {
		printf(" Input test parameter error rs_k=%d, rs_p=%d, erasures=%d\n",
		       rs_k, rs_p, num_erasures);
		usage ( argv );
	}

	printf(" (rs_k,rs_p)=(%d,%d) erasures=%d\n", rs_k, rs_p, num_erasures);

	// Total number of target erasure_patterns
	unsigned int max_erasure_patterns = compute_max_erasure_patterns(rs_k, rs_p, num_erasures);
	uint8_t* erasure_patterns; // This is actually a string array
	erasure_patterns = (uint8_t*)malloc ( sizeof ( uint8_t ) * max_erasure_patterns ); 

	// Open input erasure patterns file
	char filename[38] = "erasure_patterns_X_X.txt";
	sprintf(filename, "erasure_patterns_%d_%d.txt", rs_k, rs_p);
	fd_pattern = fopen(filename, "r");
	if ( fd_pattern == NULL ) {
		fprintf(stderr, "%s:%d: Can't open file %s\n", __FILE__, __LINE__, filename);
		return -1;
	}
	// Open output erasure patterns file
	sprintf( filename, "rs_erasure_patterns_%d_%d.c", rs_k, rs_p );
	strcpy( tmp_string, filedir );
	strcat( tmp_string, filename );
	printf("%s:%d: Opening %s\n", __FILE__, __LINE__, tmp_string);
	fd_erasure_patterns = fopen(tmp_string, "w");
	if ( fd_erasure_patterns == NULL ) {
		printf("%s:%d ERROR: can't open %s\n", __FILE__, __LINE__, filedir);   
		ret_val = -1;
		goto clean_up;
	}

	// Allocate memory
	// Total number of lines
	unsigned int tot_lines = compute_max_erasure_patterns( rs_k, rs_p, rs_p );
	// Total size of char array, each line is rs_m chars
	unsigned int size_of_erasure_strings	= sizeof(uint8_t) * tot_lines * rs_m;
	uint8_t* erasure_strings				= (uint8_t*)malloc( size_of_erasure_strings );
	
	// unsigned int size_of_erasure_patterns	= sizeof(uint8_t) * tot_lines * size_of_erasure_strings;
	// uint8_t* erasure_patterns_local 		= (uint8_t*)malloc( size_of_erasure_patterns );
	
	// read lines from file
	for ( int i = 0; i < tot_lines; i++ ){
		ret_val = fscanf( fd_pattern, "%s", &(erasure_strings[i*(rs_m+1)]) );
		if ( ret_val == 0 ) {
			fprintf(stderr, "%s:%d: Error reading from %s\n", __FILE__, __LINE__, filename);
			return -1;
		}
		// printf("%s\n", &(erasure_strings[i*(rs_m+1)]));
	}


	// Write on erasure_patterns file
	fprintf(fd_erasure_patterns, "//////////////////////////////////////////////\n"
							"// This file is autogenerated, DO NOT EDIT! //\n"
							"//////////////////////////////////////////////\n\n"
							"#include <stdint.h>\n\n");
	// Generate permutation table
	fprintf(fd_erasure_patterns, "static const uint16_t erasures_patterns_%d_%d[%d] = {\n", rs_k, rs_p, max_erasure_patterns);
	for ( unsigned int j = 0; j < max_erasure_patterns; j++ ) {
		fprintf(fd_erasure_patterns, "	0b%s%s, \n", &(erasure_strings[j * (rs_m+1)]),"u"); 
	}
	fprintf(fd_erasure_patterns, "};\n");
	fprintf(fd_erasure_patterns, "\n");

	// Convert from strings to array of integers
	unsigned long p_erasures_patterns = compute_p_erasure_patterns( rs_k, rs_p );
	uint8_t* int_array = (uint8_t*)malloc(sizeof(uint8_t) * p_erasures_patterns );
 	
	// Start from the last group, i.e. the one with P erasures
	uint8_t* offset_erasure_strings = &(erasure_strings[(rs_m+1)*(max_erasure_patterns - p_erasures_patterns)]);
	convert_bitstring_to_array ( 
								(rs_m+1), 
								p_erasures_patterns, 
								rs_p,
								offset_erasure_strings,
								int_array
	);

	// Generate permutation indexes
	fprintf(fd_erasure_patterns, "static const uint8_t erasures_%d_%d[%lu][%d] = {\n", rs_k, rs_p, p_erasures_patterns, num_erasures);
	for ( unsigned int j = 0; j < p_erasures_patterns; j++ ) {
		fprintf(fd_erasure_patterns, "\t{ ");
		for ( unsigned int i = 0; i < num_erasures; i++ ) {
			fprintf(fd_erasure_patterns, "%hhu", int_array[ (j * num_erasures) + i]);
			if ( i != (num_erasures -1) ) {
				fprintf(fd_erasure_patterns, ", ");
			}
		}
		fprintf(fd_erasure_patterns, " },\t// 0b%s\n", &(offset_erasure_strings[j * (rs_m+1)]));
	}
	fprintf(fd_erasure_patterns, "};\n");
	fprintf(fd_erasure_patterns, "\n");

close_files:
	// Close files
	fclose ( fd_erasure_patterns );

clean_up:
	// Free dynamic memory
	free ( bitstring	 );

	return ret_val;
}