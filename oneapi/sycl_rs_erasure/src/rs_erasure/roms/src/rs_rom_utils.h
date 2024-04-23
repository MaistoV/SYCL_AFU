#ifndef _RS_ROM_UTILS_H
#define _RS_ROM_UTILS_H

#include <isa-l.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <isa-l.h>	// linking against installed
#include <assert.h>

#define NUM_BASE_PATTERNS_10_1 11
#define NUM_BASE_PATTERNS_6_3 10
#define NUM_BASE_PATTERNS_3_2 2

#define MMAX 255
#define KMAX 255

int gf_gen_decode_matrix_simple(uint8_t * encode_matrix,
						uint8_t * decode_matrix,
						uint8_t * invert_matrix,
						uint8_t * temp_matrix,
						uint8_t * decode_index, uint8_t * erasure_list, 
						int nerrs, int k, int m); 
int gf_gen_decode_matrix(
				uint8_t 	*encode_matrix,
				uint8_t 	*decode_matrix,
				uint8_t 	*invert_matrix,
				uint8_t		*decode_index,
				uint8_t 	*src_err_list,	// index of blocks with erasures
				uint8_t 	*src_in_err,	// index of data blocks with erasures
				int 		nerrs, 			// length of src_err_list
				int 		nsrcerrs, 		// length of src_in_err
				int k, int m);
void print_matrix_1d(FILE* fd, int num_rows, int num_cols, unsigned char *s, const char *msg);
void print_matrix_2d(FILE* fd, int num_rows, int num_cols, unsigned char *s, const char *msg);
unsigned long compute_max_erasure_patterns( int k, int p , unsigned int num_erasures);
unsigned long compute_max_survival_vectors( int k, int p );
unsigned long compute_num_vectors_per_erasure_pattern( int k, int p );
unsigned long compute_max_erasure_patterns( int k, int p , unsigned int num_erasures );
unsigned long compute_p_erasure_patterns( int k, int p );
unsigned long compute_1_erasure_patterns( int k, int p );
int gen_1_erasure_patterns ( int k, int p, uint8_t* erasure_patterns );
int gen_erasure_patterns   ( const int k, const int p, uint8_t* erasure_patterns );
int gen_survival_patterns  ( const int k, const int p, uint8_t* survival_patterns );
void erasures_to_bitstring ( int k, int p, int num_errors, uint8_t* erasure_pattern, uint8_t* return_buffer );
void convert_binary_permutations_to_array ( const int rs_k, const int rs_p, 
											const int survival_vectors_per_erasure, 
											const uint8_t* permutations, uint8_t* survival_pattern );
void convert_bitstring_to_array ( 
									const int len_single_string, 
									const int num_substrings, 
									const int high_bits, 
									const uint8_t* bitstring,
									uint8_t* int_array 
									);

#define _1d_print
#ifdef _1d_print
#define print_matrix(fd, num_rows, num_cols, s, msg) print_matrix_1d(fd, num_rows, num_cols, s, msg)
#else
#define print_matrix(fd, num_rows, num_cols, s, msg) print_matrix_2d(fd, num_rows, num_cols, s, msg)
#endif
 
#endif // _RS_ROM_UTILS_H