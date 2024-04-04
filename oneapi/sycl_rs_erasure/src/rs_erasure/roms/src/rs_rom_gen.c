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
		"  -l <val>	Length of fragments\n"
		"  -c <0|1>	1: Use Cauchy matrix (default), 0: Use Vandermonde matrix\n"
		"  -o 		Output directory\n",
		argv[0]
	);
	exit(0);
}

int main(int argc, char *argv[]) {
	FILE *fd_rom, *fd_erasure_patterns, *fd_decode, *fd_rom_lookup;
	char filename[256];
	char filedir[256] = "./";
	char tmp_string[256];
	
	int ret_val = 0;
	// Default params
	int rs_k = 6, rs_p = 3, len = 1;	
	int num_erasures = 1; // Number of erasures
	int use_cauchy_matrix = 1;

	// Fragment buffer pointers
	uint8_t* bitstring;
	uint8_t *frag_ptrs[MMAX];
	uint8_t *src_in_err;
	// Reconstructed cells
	uint8_t *recover_outp[KMAX];	
	uint8_t *recover_srcs[KMAX];
	uint8_t *g_tbls;

	// Coefficient matrices
	uint8_t *encode_matrix, *decode_matrix;
	uint8_t *invert_matrix, *temp_matrix;
	uint8_t decode_index[MMAX];	// Which fragments are used for decoding

	int c;
	while ( ( c = getopt(argc, argv, "k:p:e:l:c:o:h") ) != -1 ) {
		switch (c) {
		case 'k':
			rs_k = atoi(optarg);
			break;
		case 'p':
			rs_p = atoi(optarg);
			break;
		case 'e':
			num_erasures = atoi(optarg);
			break;
		case 'l':
			len = atoi(optarg);
			if (len < 0) {
				usage ( argv );
			}
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
	int rs_m = rs_k + rs_p;	// Total number of cells


	// Check for valid parameters
	if (rs_m > MMAX || rs_k > KMAX || rs_m < 0 || rs_p < 1 || rs_k < 1) {
		printf(" Input test parameter error rs_m=%d, rs_k=%d, rs_p=%d, erasures=%d\n",
		       rs_m, rs_k, rs_p, num_erasures);
		usage ( argv );
	}
	if ( num_erasures > 1 ) {
		printf("Error: erasures=%d p=%d, number of erasures must be 1 \n",
			num_erasures, rs_p);
		usage( argv );
	}
	
	// Allocate coding matrices
	encode_matrix	= (uint8_t*)malloc(rs_m * rs_k);
	decode_matrix	= (uint8_t*)malloc(rs_m * rs_k);
	invert_matrix	= (uint8_t*)malloc(rs_m * rs_k);
	temp_matrix		= (uint8_t*)malloc(rs_m * rs_k);
	g_tbls		 	= (uint8_t*)malloc(rs_k * rs_p * 32);
	src_in_err 		= (uint8_t*)malloc( rs_m );
	bitstring = (uint8_t*)malloc( sizeof(uint8_t) * ( rs_k + rs_p + 1 ) );

	if ( encode_matrix == NULL || decode_matrix == NULL
	    || invert_matrix == NULL || temp_matrix == NULL ) {
		printf("%s:%d Test failure! Error with (uint8_t*)malloc\n", __FILE__, __LINE__);
		goto clean_up;;
	}
	// Allocate the src & parity buffers
	for ( unsigned int i = 0; i < rs_m; i++ ) {
		if (NULL == (frag_ptrs[i] = (uint8_t*)malloc(len))) {
			printf("%s:%d (uint8_t*)malloc error: Fail\n", __FILE__, __LINE__);
			goto clean_up;;
		}
	}

	// Allocate buffers for recovered data
	for ( unsigned int i = 0; i < rs_p; i++ ) {
		if (NULL == (recover_outp[i] = (uint8_t*)malloc(len))) {
			printf("%s:%d (uint8_t*)malloc error: Fail\n", __FILE__, __LINE__);
			goto clean_up;;
		}
	}

	// Fill sources with random data
	for ( int i = 0; i < rs_k; i++ )
		for ( int j = 0; j < len; j++)
			frag_ptrs[i][j] = rand();

	printf(" encode (rs_m,rs_k,rs_p)=(%d,%d,%d) erasures=%d len=%d\n", rs_m, rs_k, rs_p, num_erasures, len);

	// Pick an encode matrix. A Cauchy matrix is a good choice as even
	// large rs_k are always invertable keeping the recovery rule simple.
	if ( use_cauchy_matrix ){
		gf_gen_cauchy1_matrix(encode_matrix, rs_m, rs_k);
	}
	else {
		gf_gen_rs_matrix(encode_matrix, rs_m, rs_k);
	}
	// print_matrix(rs_m, rs_k, encode_matrix, "encode_matrix");

	// Generate g_tbls
	ec_init_tables(rs_k, rs_p, &encode_matrix[rs_k * rs_k], g_tbls);
	// Generate EC parity blocks from sources
	ec_encode_data(len, rs_k, rs_p, g_tbls, frag_ptrs, &frag_ptrs[rs_k]);
	
	// Total number of target erasure_patterns
	unsigned long max_erasure_patterns = compute_1_erasure_patterns( rs_k, rs_p );
	uint8_t* erasure_patterns;
	erasure_patterns = (uint8_t*)malloc ( sizeof ( uint8_t ) * max_erasure_patterns ); 
	// Generate erasure_patterns
	// This wll generate all the erasure patterns, but most of the code below relies only on the trivial ones
	// with a single erasure, except for rs_erasure_patterns_X_X.c generation
	ret_val = gen_1_erasure_patterns ( rs_k, rs_p, erasure_patterns );
	if ( ret_val != 0 ) {
		printf("%s:%d ERROR: can't generate erasure_patterns\n", __FILE__, __LINE__);
		goto clean_up;
	}

	// Total number of survival patterns
	unsigned long max_survival_patterns;
	max_survival_patterns = compute_max_survival_vectors( rs_k, rs_p );
	uint8_t* survival_patterns;
	survival_patterns = (uint8_t*)malloc ( sizeof ( uint8_t ) * max_survival_patterns ); 
	// Generate survival_patterns
	ret_val = gen_survival_patterns ( rs_k, rs_p, survival_patterns );
	if ( ret_val != 0 ) {
		printf("%s:%d ERROR: can't generate survival_patterns\n", __FILE__, __LINE__);
		goto clean_up;
	}

/*
	// Open erasure patterns file
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
	// Write on erasure_patterns file
	fprintf(fd_erasure_patterns, "//////////////////////////////////////////////\n"
							"// This file is autogenerated, DO NOT EDIT! //\n"
							"//////////////////////////////////////////////\n\n"
							"#include <stdint.h>\n\n");
	// Generate permutation table
	fprintf(fd_erasure_patterns, "static const uint16_t erasures_patterns_%d_%d[%ld] = {\n", rs_k, rs_p, max_erasure_patterns);
	for ( unsigned int j = 0; j < max_erasure_patterns; j++ ) {
		fprintf(fd_erasure_patterns, "	0b%s%s, ", &(erasure_patterns[ j * num_erasures ]),"u");	
	}
	fprintf(fd_erasure_patterns, "};\n");
	fprintf(fd_erasure_patterns, "\n");

	fprintf(fd_erasure_patterns, "static const uint8_t erasures_%d_%d[%ld][%d] = {\n", rs_k, rs_p, max_erasure_patterns, num_erasures);
	for ( unsigned int j = 0; j < max_erasure_patterns; j++ ) {
		uint8_t* erasure_list = &(erasure_patterns[ j * num_erasures ]);
		fprintf(fd_erasure_patterns, "\t{ ");
		for ( unsigned int i = 0; i < num_erasures; i++ ) {
			fprintf(fd_erasure_patterns, "%hhu", erasure_list[i]);
			if ( i != (num_erasures -1) ) {
				fprintf(fd_erasure_patterns, ", ");
			}
		}
		fprintf(fd_erasure_patterns, " },\t// %d\n", j);

	}
	fprintf(fd_erasure_patterns, "};\n");
	fprintf(fd_erasure_patterns, "\n");
*/

	// Allocate strings
	int num_vectors_per_erasure_pattern = compute_num_vectors_per_erasure_pattern (rs_k, rs_p);
	uint8_t* decode_index_bitstring;
	decode_index_bitstring = (uint8_t*)malloc( sizeof(uint8_t) * ( rs_m + 1 ) * num_vectors_per_erasure_pattern * max_erasure_patterns );

	// Open rom file
	sprintf( filename, "rs_rom_%d_%d.c", rs_k, rs_p );
	strcpy( tmp_string, filedir );
	strcat( tmp_string, filename );
	printf("%s:%d: Opening %s\n", __FILE__, __LINE__, tmp_string);
	fd_rom = fopen(tmp_string, "w");
	if ( fd_rom == NULL ) {
		printf("%s:%d Error: can't open %s\n", __FILE__, __LINE__, filedir);
		ret_val = -1;
		goto clean_up;
	}
	// Open decode file
	sprintf( filename, "rs_decode_%d_%d.c", rs_k, rs_p );
	strcpy( tmp_string, filedir );
	strcat( tmp_string, filename );
	printf("%s:%d: Opening %s\n", __FILE__, __LINE__, tmp_string);
	fd_decode = fopen(tmp_string, "w");
	if ( fd_decode == NULL ) {
		printf("%s:%d Error: can't open %s\n", __FILE__, __LINE__, filedir);
		ret_val = -1;
		goto clean_up;
	}
	// Write on rom file
	fprintf(fd_rom, "//////////////////////////////////////////////\n"
							"// This file is autogenerated, DO NOT EDIT! //\n"
							"//////////////////////////////////////////////\n\n"
							"#include <stdint.h>\n\n");
	// Write on decode file
	fprintf(fd_decode, "//////////////////////////////////////////////\n"
							"// This file is autogenerated, DO NOT EDIT! //\n"
							"//////////////////////////////////////////////\n\n"
							"#include <stdint.h>\n\n");
	// Generate array of decoding matrices
	fprintf(fd_rom, "static const uint8_t DECMAT_ROM_%d_%d[%ld][%d] = {\n", rs_k, rs_p, max_erasure_patterns*num_vectors_per_erasure_pattern, rs_k);
	fprintf(fd_rom, "\t// decode_matrix, erasure_pattern, survival_pattern, decode_index\n");
	fprintf(fd_decode, "static const uint8_t decode_index_%d_%d[%ld][%d][%d] = {\n", rs_k, rs_p, max_erasure_patterns, num_vectors_per_erasure_pattern, rs_k);
	for ( unsigned int j = 0; j < max_erasure_patterns; j++ ) {
		uint8_t* erasure_list = &(erasure_patterns[ j * num_erasures ]);

		// Find a decode matrix to regenerate all erasures from remaining frags
		// ret_val = gf_gen_decode_matrix_simple(encode_matrix, decode_matrix,
		// 				invert_matrix, temp_matrix, decode_index,
		// 				erasure_list, num_erasures, rs_k, rs_m);
		
		int nsrcerrs = 0;
		if ( erasure_list[0] < rs_k ) {
			nsrcerrs = 1;
		}
		fprintf(fd_decode,"\t{\n");
		// For each possible survival pattern for this erasure parttern
		for ( unsigned int v = 0; v < num_vectors_per_erasure_pattern; v++ ) {
			unsigned int linear_index = j*num_vectors_per_erasure_pattern + v;
			// One-hot init
			for ( int i = 0; i < rs_m; i++ ){
				src_in_err[i] = 1;
			}
			unsigned int s = 0;
			for ( unsigned int i = 0; i < rs_k; i++ ) {
				uint8_t index = survival_patterns[ linear_index*rs_k + i];
				src_in_err[index] = 0;
			}

			// Generate and save decmat & co.
			ret_val = gf_gen_decode_matrix(encode_matrix, decode_matrix,
							invert_matrix, decode_index,
							erasure_list,	// index of erased blocks
							src_in_err,		// mask of data blocks with erasures
							num_erasures, 	// length of src_err_list
							nsrcerrs, 		// length of src_in_err
							rs_k, rs_m);
			if ( ret_val != 0 ) {
				printf("%s:%d: Error while calling gf_gen_decode_matrix\n", __FILE__, __LINE__);
				goto close_files;
			}

			// Save decode index in bitstring for later
			erasures_to_bitstring(rs_k, rs_p, rs_k, decode_index, 
				&(decode_index_bitstring[linear_index*(rs_m+1)]));

			fprintf(fd_decode, "\t\t{");
			for ( int i = 0; i < rs_k; i++){
				fprintf(fd_decode, " %hhu", decode_index[i]);
				if ( i != (rs_k - 1) ) {
					fprintf(fd_decode, ",");
				}	
			}
			// fprintf(fd_decode, " }");
			fprintf(fd_decode,( v < num_vectors_per_erasure_pattern -1 ) ? " }," : " }\t");
			// if ( v != (num_vectors_per_erasure_pattern - 1) ) {
			// 	fprintf(fd_decode, ",");
			// }	
			fprintf(fd_decode, "\t// ");
			for ( unsigned int i = 0; i < num_erasures; i++ ) {
				fprintf(fd_decode, "%hhu, %hhu ", erasure_list[i], v);
			}	
			fprintf(fd_decode, ", 0b%su", &(decode_index_bitstring[linear_index*(rs_m+1)]));
			fprintf(fd_decode, "\n");
			


			// print_matrix(stdout, num_erasures, rs_k, decode_matrix, "\n\t");
			print_matrix(fd_rom, num_erasures, rs_k, decode_matrix, "\t");
			if ( linear_index != (max_erasure_patterns*num_vectors_per_erasure_pattern - 1) ) {
				fprintf(fd_rom, ",");
			}
			fprintf(fd_rom, "\t// ");
			erasures_to_bitstring(rs_k, rs_p, num_erasures, erasure_list, bitstring);
			fprintf(fd_rom, "0b%s%s,", bitstring,"u");

			// erasures_to_bitstring(rs_k, rs_p, rs_k, decode_index, bitstring);
			// fprintf(fd_rom, " 0b%s ,", bitstring);
			fprintf(fd_rom, " 0b%s ,", &(decode_index_bitstring[linear_index*(rs_m+1)]));
			fprintf(fd_rom, " {");
			for ( int i = 0; i < rs_k; i++ ) {
				fprintf(fd_rom, " %hhu", decode_index[i]);
			}
			fprintf(fd_rom, " }\n");


			// Pack recovery array pointers as list of valid fragments
			for ( int i = 0; i < rs_k; i++ )
				recover_srcs[i] = frag_ptrs[decode_index[i]];

			// Recover data
			ec_init_tables(rs_k, num_erasures, decode_matrix, g_tbls);
			ec_encode_data(len, rs_k, num_erasures, g_tbls, recover_srcs, recover_outp);

			// Check that recovered buffers are the same as original
			for ( unsigned int i = 0; i < num_erasures; i++ ) {
				ret_val = memcmp(recover_outp[i], frag_ptrs[erasure_list[i]], len);
				if ( ret_val ) {
					printf(" Fail erasure recovery %d, frag %d\n", i, erasure_list[i]);
					goto close_files;
				}
			}

		}
		fprintf(fd_decode,( j < max_erasure_patterns -1 ) ? "\t},\n" : "\t}\n");
	}

	fprintf(fd_rom, "};\n");	
	fprintf(fd_decode, "};\n\n");

	// Write decode bitstrings
	fprintf(fd_decode, "static const uint16_t decode_index_%d_%d_bitstring[%ld][%d] = {\n", rs_k, rs_p, max_erasure_patterns,num_vectors_per_erasure_pattern);
	for ( unsigned int j = 0; j < max_erasure_patterns; j++ ) {
		uint8_t* erasure_list = &(erasure_patterns[ j * num_erasures ]);
		// For each possible survival pattern for this erasure parttern
		fprintf(fd_decode, "\t{\n");
		for ( unsigned int v = 0; v < num_vectors_per_erasure_pattern; v++ ) {
			unsigned int linear_index = j*num_vectors_per_erasure_pattern + v;
			fprintf(fd_decode, "\t\t0b%su", &(decode_index_bitstring[linear_index*(rs_m+1)]));
			if ( v != (num_vectors_per_erasure_pattern - 1) ) {
				fprintf(fd_decode, ",");
			}	
			fprintf(fd_decode, "\t// ");
			for ( unsigned int i = 0; i < num_erasures; i++ ) {
				fprintf(fd_decode, "%hhu, %hhu", erasure_list[i], v);
			}	
			fprintf(fd_decode, ", {");
			for ( int i = 0; i < rs_k; i++){
				fprintf(fd_decode, " %hhu", decode_index[i]);
				if ( i != (rs_k - 1) ) {
					fprintf(fd_decode, ",");
				}	
			}
			fprintf(fd_decode, " }\n");
		}
		fprintf(fd_decode,( j < max_erasure_patterns -1 ) ? "\t},\n" : "\t}\n");
	}
	fprintf(fd_decode, "};\n");
	
	// Generate look up table	
	// Open rom lookup file
	sprintf( filename, "rs_rom_lookup_%d_%d.c", rs_k, rs_p );
	strcpy( tmp_string, filedir );
	strcat( tmp_string, filename );
	printf("%s:%d: Opening %s\n", __FILE__, __LINE__, tmp_string);
	fd_rom_lookup = fopen(tmp_string, "w");
	if ( fd_rom_lookup == NULL ) {
		printf("%s:%d Erro{r: can't open %s\n", __FILE__, __LINE__, filedir);
		ret_val = -1;
		goto clean_up;
	}

	// Write on erasure_patterns file
	fprintf(fd_rom_lookup, "//////////////////////////////////////////////\n"
							"// This file is autogenerated, DO NOT EDIT! //\n"
							"//////////////////////////////////////////////\n\n"
							"#include <stdint.h>\n\n"
							"uint16_t rs_%d_%d_rom_lookup( uint16_t erasure_pattern, uint16_t survival_pattern  ) {\n", rs_k, rs_p);
	fprintf(fd_rom_lookup, "	uint16_t ret_val = 0;\n");
	uint16_t mask = 0xffff;
	if ( rs_k == 3 && rs_p == 2 ) {
		mask = 0x001f;
	}
	if ( rs_k == 6 && rs_p == 3 ) {
		mask = 0x01ff;
	}
	if ( rs_k == 10 && rs_p == 4 ) {
		mask = 0x3fff;
	}
	fprintf(fd_rom_lookup, "\tswitch ( erasure_pattern & (uint16_t)0x%04xu ) {\n", mask );

	for ( unsigned int j = 0; j < max_erasure_patterns; j++ ) {
		uint8_t* erasure_list = &(erasure_patterns[ j * num_erasures ]);
		erasures_to_bitstring(rs_k, rs_p, num_erasures, erasure_list, bitstring);
		fprintf(fd_rom_lookup, "\t\tcase 0b%su:\n", bitstring );	
		fprintf(fd_rom_lookup, "\t\t\tswitch ( survival_pattern & (uint16_t)0x%04xu ) {\n", mask );
		for ( unsigned int v = 0; v < num_vectors_per_erasure_pattern; v++ ) {
			unsigned int linear_index = j*num_vectors_per_erasure_pattern + v;
			fprintf(fd_rom_lookup, "\t\t\tcase 0b%s%s:\n", &(decode_index_bitstring[linear_index*(rs_m+1)]), "u" );	
			fprintf(fd_rom_lookup, "\t\t\t\tret_val = %d;\n", linear_index);	
			fprintf(fd_rom_lookup, "\t\t\t\tbreak;\n");	
		}
		fprintf(fd_rom_lookup, "\t\t\tdefault:\n");
		fprintf(fd_rom_lookup, "\t\t\t\t// Error\n");
		fprintf(fd_rom_lookup, "\t\t\t\t#ifdef NO_SYCL\n");
		fprintf(fd_rom_lookup, "\t\t\t\tprintf(\"%%s:%%d ERROR: Unsupported survival pattern 0x%%04x\\n\", __FILE__, __LINE__, survival_pattern);\n");
		fprintf(fd_rom_lookup, "\t\t\t\t#endif // NO_SYCL\n");
		fprintf(fd_rom_lookup, "\t\t\t\tret_val = -1;\n");
		fprintf(fd_rom_lookup, "\t\t\t\tbreak;\n");	
		fprintf(fd_rom_lookup, "\t\t\t}\n");	
		fprintf(fd_rom_lookup, "\t\t\tbreak;\n");	
	}	
	fprintf(fd_rom_lookup, "\tdefault:\n");
	fprintf(fd_rom_lookup, "\t\t// Error\n");
	fprintf(fd_rom_lookup, "\t\t\t\t#ifdef NO_SYCL\n");
	fprintf(fd_rom_lookup, "\t\tprintf(\"%%s:%%d ERROR: Unsupported erasure pattern 0x%%04x\\n\", __FILE__, __LINE__, erasure_pattern);\n");
	fprintf(fd_rom_lookup, "\t\t\t\t#endif // NO_SYCL\n");
	fprintf(fd_rom_lookup, "\t\tret_val = -1;\n");
	fprintf(fd_rom_lookup, "\tbreak;\n");	
	fprintf(fd_rom_lookup, "\t}\n");	
	fprintf(fd_rom_lookup, "\t\n	return ret_val;\n");	
	fprintf(fd_rom_lookup, "}\n");	

close_files:
	// Close files
	fclose ( fd_rom		 	 );
	fclose ( fd_rom_lookup	 );
	fclose ( fd_decode	 	 );
	// fclose ( fd_erasure_patterns );

clean_up:
	// Free dynamic memory
	free ( encode_matrix );
	free ( decode_matrix );
	free ( invert_matrix );
	free ( temp_matrix 	 );
	free ( g_tbls		 );
	free ( bitstring	 );
	for ( unsigned int i = 0; i < rs_m; i++ ) {
		free ( frag_ptrs[i] );
	}
	for ( unsigned int i = 0; i < rs_p; i++ ) {
		free (recover_outp[i] );
	}

	return ret_val;
}