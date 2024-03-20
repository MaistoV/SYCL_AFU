#include "rs_rom_utils.h"


void gen_err_list(
			unsigned char *src_err_list, // index of blocks with erasures
			unsigned char *src_in_err, 	 // index of data blocks with erasures
			int *pnerrs, 				 // length of src_err_list
			int *pnsrcerrs, 			 // length of src_in_err
			int k, int m)
{
	int i, err;
	int nerrs = 0, nsrcerrs = 0;

	for (i = 0, nerrs = 0, nsrcerrs = 0; i < m && nerrs < m - k; i++) {
		err = 1 & rand();
		printf("%d \n", err);
		src_in_err[i] = err;
		if (err) {
			src_err_list[nerrs++] = i;
			if (i < k) {
				nsrcerrs++;
			}
		}
	}
	if (nerrs == 0) {	// should have at least one error
		printf("No error generated, apparing");
		while ((err = (rand() % KMAX)) >= m) ;
		src_err_list[nerrs++] = err;
		src_in_err[err] = 1;
		if (err < k)
			nsrcerrs = 1;
	}
	*pnerrs = nerrs;
	*pnsrcerrs = nsrcerrs;
	return;
}


/*
 * Generate decode matrix from encode matrix and erasure list
 *
 */

int gf_gen_decode_matrix_simple(uint8_t * encode_matrix,
						uint8_t * decode_matrix,
						uint8_t * invert_matrix,
						uint8_t * temp_matrix,
						uint8_t * decode_index, uint8_t * erasure_list, 
						int nerrs, int k, int m) {
	int p;
	int nsrcerrs;
	uint8_t s, *b = temp_matrix;
	uint8_t frag_in_err[MMAX];

	memset(frag_in_err, 0, sizeof(frag_in_err));

	// Order the fragments in erasure for easier sorting
	nsrcerrs = 0;
	for ( unsigned int i = 0; i < nerrs; i++ ) {
		if (erasure_list[i] < k)
			nsrcerrs++;
		frag_in_err[erasure_list[i]] = 1;
	}

	// Construct b (matrix that encoded remaining frags) by removing erased rows
	for ( unsigned int i = 0, r = 0; i < k; i++, r++ ) {
		while (frag_in_err[r])
			r++;
		for ( unsigned int j = 0; j < k; j++ )
			b[k * i + j] = encode_matrix[k * r + j];
		decode_index[i] = r;
	}

	// Invert matrix to get recovery matrix
	if (gf_invert_matrix(b, invert_matrix, k) < 0)
		return -1;

	// Get decode matrix with only wanted recovery rows
	for ( unsigned int i = 0; i < nerrs; i++ ) {
		if (erasure_list[i] < k)	// A src err
			for ( unsigned int j = 0; j < k; j++ )
				decode_matrix[k * i + j] =
				    invert_matrix[k * erasure_list[i] + j];
	}

	// For non-src (parity) erasures need to multiply encode matrix * invert
	for (p = 0; p < nerrs; p++ ) {
		if (erasure_list[p] >= k) {	// A parity err
			for ( unsigned int i = 0; i < k; i++ ) {
				s = 0;
				for ( unsigned int j = 0; j < k; j++ )
					s ^= gf_mul(invert_matrix[j * k + i],
						    encode_matrix[k * erasure_list[p] + j]);
				decode_matrix[k * p + i] = s;
			}
		}
	}
	return 0;
}

// Generate decode matrix from encode matrix
#define NO_INVERT_MATRIX -2
int gf_gen_decode_matrix(
				uint8_t 	*encode_matrix,
				uint8_t 	*decode_matrix,
				uint8_t 	*invert_matrix,
				uint8_t		*decode_index,
				uint8_t 	*src_err_list,	// index of blocks with erasures
				uint8_t 	*src_in_err,	// index of data blocks with erasures
				int 		nerrs, 			// length of src_err_list
				int 		nsrcerrs, 		// length of src_in_err
				int k, int m)
{
	int i, j, p;
	int r;
	uint8_t *backup, *b, s;
	int incr = 0;

	b = (uint8_t*) malloc(MMAX * KMAX);
	backup = (uint8_t*)malloc(MMAX * KMAX);

	if (b == NULL || backup == NULL) {
		printf("Test failure! Error with malloc\n");
		free(b);
		free(backup);
		return -1;
	}
	// Construct matrix b by removing error rows
	for (i = 0, r = 0; i < k; i++, r++) {
		while (src_in_err[r]){
			r++;
		}
		for (j = 0; j < k; j++) {
			b[k * i + j] = encode_matrix[k * r + j];
			backup[k * i + j] = encode_matrix[k * r + j];
		}
		decode_index[i] = r;
	}
	incr = 0;
	while (gf_invert_matrix(b, invert_matrix, k) < 0) {
		if (nerrs == (m - k)) {
			free(b);
			free(backup);
			fprintf(stderr, "%s:%d NO_INVERT_MATRIX\n", __FILE__, __LINE__);
			return NO_INVERT_MATRIX;
		}
		incr++;
		memcpy(b, backup, MMAX * KMAX);
		for (i = nsrcerrs; i < nerrs - nsrcerrs; i++) {
			if (src_err_list[i] == (decode_index[k - 1] + incr)) {
				// skip the erased parity line
				incr++;
				continue;
			}
		}
		if (decode_index[k - 1] + incr >= m) {
			free(b);
			free(backup);
			fprintf(stderr, "%s:%d NO_INVERT_MATRIX %d\n", __FILE__, __LINE__, decode_index[k - 1] + incr);
			return NO_INVERT_MATRIX;
		}
		decode_index[k - 1] += incr;
		for (j = 0; j < k; j++)
			b[k * (k - 1) + j] = encode_matrix[k * decode_index[k - 1] + j];

	};

	for (i = 0; i < nsrcerrs; i++) {
		for (j = 0; j < k; j++) {
			decode_matrix[k * i + j] = invert_matrix[k * src_err_list[i] + j];
		}
	}
	/* src_err_list from encode_matrix * invert of b for parity decoding */
	for (p = nsrcerrs; p < nerrs; p++) {
		for (i = 0; i < k; i++) {
			s = 0;
			for (j = 0; j < k; j++)
				s ^= gf_mul(invert_matrix[j * k + i],
					    encode_matrix[k * src_err_list[p] + j]);

			decode_matrix[k * p + i] = s;
		}
	}
	free(b);
	free(backup);
	return 0;
}

// Print matrix in a row, C-like fashion
void print_matrix_1d(FILE* fd, int num_rows, int num_cols, unsigned char *s, const char *msg){
	fprintf(fd, "%s {", msg);
	for ( unsigned int i = 0; i < num_rows*num_cols; i++ ) {
		fprintf(fd, " %3d", 0xff & s[i]);
		if ( i != ( num_cols*num_rows -1 ) ) {
			fprintf(fd, ",");
		}
	}
	fprintf(fd, " }");
}

// Print matrix as 2d vector
void print_matrix_2d(FILE* fd, int num_rows, int num_cols, unsigned char *s, const char *msg){
	fprintf(fd, "%s \n", msg);
	for (int i = 0; i < num_rows; i++ ) {
		fprintf(fd, "%3d- ", i);
		for (int j = 0; j < num_cols; j++ ) {
			fprintf(fd, " %3d", 0xff & s[j + (i * num_cols)]);
		}
		fprintf(fd, "\n");
	}
	fprintf(fd, "\n");
}

// Factorial of n
unsigned long factorial ( int n ) {
	// Input sanitizatioon
	if ( n < 0 ) {
		printf("Error: %s is defined on non-negative integers\n", __func__);
		return -1;
	}
	// Base case: 0! = 1! = 1
	if ( n <= 1 ) {
		return 1;
	}
	// Recursion
	return ( n * factorial(n-1) );
}

// Binomial coeffcient 
unsigned long binom ( int n, int m ) {
	return ( factorial( n ) / factorial(m) / factorial(n-m) );
}

// Compute ( m + k )! / ( p! * k! )
unsigned long compute_max_erasure_patterns( int k, int p ) {
	return ( factorial( k + p ) / factorial(k) / factorial(p) );
}

// Compute ( ( k + p ) * binom( k+p-1, k ) * k )
unsigned long compute_max_survival_vectors( int k, int p ) {
	return ( factorial( k + p ) / factorial(k-1) / factorial(p-1) );
	// return (( k + p ) * binom( k+p-1, k ) * k );
}

unsigned long compute_num_vectors_per_erasure_pattern( int k, int p ) {
	return ( binom( k+p-1, k ) );
}

// NOTE: caller must allocate space for return buffer
void erasures_to_bitstring ( int k, int p, int num_errors, uint8_t* erasure_pattern, uint8_t* return_buffer ) {
	// Use chars (bytes) as bits
	int rs_length = k + p;

	// Reset all the bytes
	for ( unsigned int i = 0; i < rs_length; i++ ){
		return_buffer[i] = '0';
	}	
	// Append string terminator
	return_buffer[rs_length] = '\0';
	
	// Set only the bytes corresponding to the target bit
	for ( unsigned int i = 0; i < num_errors; i++ ){
		return_buffer[ rs_length -1 - erasure_pattern[i] ] = '1'; 
	}

}

// for qsort
int comp (const void * a, const void * b) {
	unsigned char f = *((unsigned char *)a);
	unsigned char s = *((unsigned char *)b);
	if (f > s) return  1;
	if (f < s) return -1;
	return 0;
}

// NOTE: this does not need to be efficient
// NOTE: only some codes are implemented for now
// TODO: implement base HDFS RS schemas: RS-3-2-1024k, RS-6-3-1024k, RS-10-4-1024k.
int gen_p_erasure_erasure_patterns ( int k, int p, uint8_t* erasure_patterns ) {
	// Check inputs
	if ( !(	( k == 6 && p == 3 ) ||
			( k == 3 && p == 2 ) ||
			( k == 10 && p == 1 )	
			)
		) {
		printf("%s: k:p = %d:%d, only 10:1, 6:3 and 3:2 are implemented for now, returning...\n", __func__, k, p );
		return -1;
	}	
	if ( NULL == erasure_patterns ){
		printf("%s: erasure_patterns NULL pointer\n", __func__ );
		return -1;
	}

	uint8_t base_patterns_3_2 [NUM_BASE_PATTERNS_3_2][2] = {{1, 0}, {2, 0}};
	// uint8_t base_patterns_6_1	[6+1] 	= {6, 5, 4, 3, 2, 1, 0};
	// uint8_t base_patterns_6_2	[4][2] 	= { {1, 0}, {2, 0}, {3,0}, {4,0} };
	uint8_t base_patterns_6_3 [NUM_BASE_PATTERNS_6_3][3] = {	{2, 1, 0},
														{3, 2, 0},		
														{3, 4, 0},       
														{4, 5, 0},      
														{5, 6, 0},       
														{6, 7, 0},       
														{4, 2, 0},       
														{5, 3, 0},       
														{5, 2, 0},       
														{6, 3, 0}	// This will show periodic redundancy over the shifts				
													};
	// uint8_t base_patterns_10_1 [NUM_BASE_PATTERNS_10_4][4]  = { ... }
	uint8_t base_patterns_10_1 [NUM_BASE_PATTERNS_10_1][1]  = { {0}, {1}, {2}, {3}, {4}, {5}, 
																{6}, {7}, {8}, {9}, {10} };

	// Mux target 
	uint8_t* target_base_pattern;
	int num_base_patterns;
	#define K_P_CONCAT(k,p)  (unsigned long)k << 32 | (unsigned long)p
	unsigned long k_p =	K_P_CONCAT(k,p);
	switch ( k_p ) {
	case K_P_CONCAT(3, 2):
		target_base_pattern = (uint8_t*) base_patterns_3_2;
		num_base_patterns = NUM_BASE_PATTERNS_3_2;
		break;
	case K_P_CONCAT(6, 3):
		target_base_pattern = (uint8_t*) base_patterns_6_3;
		num_base_patterns = NUM_BASE_PATTERNS_6_3;
		break;
	// case K_P_CONCAT(10, 4):
		// target_base_pattern = (uint8_t*) base_patterns_10_4;
		// num_base_patterns = NUM_BASE_PATTERNS_10_4;
		// break;
	case K_P_CONCAT(10, 1):
		target_base_pattern = (uint8_t*) base_patterns_10_1;
		num_base_patterns = NUM_BASE_PATTERNS_10_1;
		break;
	default:
		return -1;
		break;
	}

	// For each base pattern
	for ( unsigned int base_pattern_index = 0; base_pattern_index < num_base_patterns; base_pattern_index++ ) {
		
		// Skip shifts in trivial case of p == 1
		if ( p == 1 ) {
			// printf("{ ");
			erasure_patterns[base_pattern_index] = target_base_pattern[base_pattern_index];
			// printf("%hhu ", erasure_patterns[base_pattern_index]);
			// printf("} ");
		}
		else{
			// Shift the base pattern k+p times
			#define NUM_SHIFTS ( k + p )
			for ( unsigned int shift_amount = 0; shift_amount < NUM_SHIFTS; shift_amount++ ){
				// Skip redundant shifts for last periodic pattern
				// NOTE: in case of 6:3 only one pattern is periodic, with larger codes there will be probably more
				// NOTE: There is no periodic pattern for 3:2
				if ( k == 6 && p == 3 && 
						base_pattern_index == ( num_base_patterns -1 ) && 
						shift_amount > 2 
					){
					break;
				}
					
				// Shifting of l positions corresponds to addding l to each element
				for ( unsigned int j = 0; j < p; j++ ) {
					erasure_patterns[((base_pattern_index * NUM_SHIFTS + shift_amount) * p) + j] = 
						( target_base_pattern[(base_pattern_index * p) + j] + shift_amount ) % NUM_SHIFTS;
				}
				
				qsort (&(erasure_patterns[((base_pattern_index * NUM_SHIFTS + shift_amount) * p) ]), 
						p, sizeof(uint8_t), comp);

				// printf("{ ");
				// for ( unsigned int j = 0; j < p; j++ ) {
				// 	printf("%hhu ", erasure_patterns[((base_pattern_index * NUM_SHIFTS + shift_amount) * p) + j]);
				// }
				// printf("} ");
			}
		}	
		// printf("\n");

	}

	return 0;
}
/*
int gen_2_erasure_erasure_patterns ( int k, int p, uint8_t erasure_patterns [][2] ) {
	
	// unsigned int num_base_patterns;
	// unsigned long max_erasure_patterns = compute_max_erasure_patterns( k, p );
	
	if ( k != 6 || p != 3 ) {
		printf("%s: only k:p = 6:3 is implemented for now, returning\n", __func__ );
		return -1;
	}

	// uint8_t base_patterns_1_erasure  [6+1] 	= {6, 5, 4, 3, 2, 1, 0};
	uint8_t base_patterns_2_erasures [4][2] 	= { {1, 0}, {2, 0}, {3,0}, {4,0} };
	#define NUM_BASE_PATTERNS 4 
	// For each base pattern
	for ( int base_pattern_index = 0; base_pattern_index < NUM_BASE_PATTERNS; base_pattern_index++ ) {
		// Shift the base pattern k+p-1 times
		#define NUM_SHIFTS ( k+p-1 )
		for ( int shift_amount = 0; shift_amount < NUM_SHIFTS; shift_amount++ ){
			// Shifting of l positions corresponds to addding l to each element
			for ( int j = 0; j < 2; j++ ) {
				erasure_patterns[base_pattern_index * NUM_SHIFTS][j] = 
					( base_patterns_2_erasures[base_pattern_index][j] + shift_amount ) % (k+p+1);
			}
			printf("{%hu, %hu} ",  erasure_patterns[base_pattern_index * NUM_SHIFTS][0],
										erasure_patterns[base_pattern_index * NUM_SHIFTS][1]
				);
		}
		printf("\n");

	}

	return 0;
}
*/

// In case of a single erasure, the erasure pattern is very simple
int gen_1_erasure_patterns ( int k, int p, uint8_t* erasure_patterns ) {
	
	// Check inputs
	if ( NULL == erasure_patterns ){
		printf("%s: erasure_patterns NULL pointer\n", __func__ );
		return -1;
	}

	// In case of a single erasure, the erasure pattern is very simple
	for ( int i = 0; i < k + p; i++ ){
		erasure_patterns[i] = i;
		// printf("{ %hhu }\n", erasure_patterns[i]);
	}

	return 0;
}


void convert_binary_permutations_to_array ( const int rs_k, const int rs_p, const int survival_vectors_per_erasure, 
											const uint8_t* permutations, uint8_t* survival_pattern ){

	int survival_pattern_length, rs_m;
	survival_pattern_length = rs_k + rs_p - 1;
	rs_m = rs_k + rs_p;
	// FILE* fd = stdout;

	// Loop over possible single-block erasures
	for ( int i = 0; i < rs_m; i++ ){
		// fprintf(fd,"\t{\n");
		// Loop over ((k+rs_p-1) k) permutations
		for ( int j = 0; j < survival_vectors_per_erasure; j++ ){
			// Loop over chars in strings
			int l = 0;
			// fprintf(fd,"\t\t{");
			for ( int c = 0; c < survival_pattern_length; c++ ) {
				int incr = 0;
				if ( i <= c ){
					incr = 1;
				}
				if ( permutations[j*rs_m + c] == '1'){
					// survival_patterns_local[i][j][k] = c + incr;
					// // fprintf(fd," %hhu", survival_patterns_local[i][j][l]);
					survival_pattern[(i*survival_vectors_per_erasure + j)*rs_k + l] = c + incr;
					// fprintf(fd," %hhu", c + incr);
					l++;
					if ( l < rs_k ) {
						// fprintf(fd,",");
					}
				}
			}
			// fprintf(fd,( j < survival_vectors_per_erasure ) ? " }," : " }");

			// fprintf(fd,"\t // %d, %d\n", i, j);
		}
		// fprintf(fd,( i < survival_pattern_length ) ? "\t},\n" : "\t}\n");
	}
	// fprintf(fd,"};\n");

}


int gen_survival_patterns (  const int rs_k, const int rs_p, const uint8_t* erasure_patterns, uint8_t* survival_patterns ){
	int survival_vectors_per_erasure;
	FILE* fd_pattern;
	int rs_m = rs_k+rs_p;
	int ret_val;

	if ( !((rs_k == 3) && (rs_p == 2))
			&& !((rs_k == 6) && (rs_p == 3))
			&& !((rs_k == 10) && (rs_p == 4))
			 ){
		fprintf(stderr, "%s:%d: K=%d and P=%d not supported\n", __FILE__, __LINE__, rs_k, rs_p);
		return -1;
	}

	// Open permutation file
	// NOTE: permutations are generated in a C++ program to avoid the use of C++ here
	char filename[37] = "src/roms/survival_patterns_X_X.txt";
	sprintf(filename, "src/roms/survival_patterns_%d_%d.txt", rs_k, rs_p);
	fd_pattern = fopen(filename, "r");
	if ( fd_pattern == NULL ) {
		fprintf(stderr, "%s:%d: Can't open file %s\n", __FILE__, __LINE__, filename);
		return -1;
	}

	// Allocate memory
	int size_of_permutations_strings	= sizeof(uint8_t) * binom(rs_m-1, rs_k) * (rs_m-1+1);
	int size_of_survival_patterns 		= sizeof(uint8_t) * compute_max_survival_vectors( rs_k, rs_p );

	uint8_t* permutations_strings		= (uint8_t*)malloc( size_of_permutations_strings );
	uint8_t* survival_patterns_local 	= (uint8_t*)malloc( size_of_survival_patterns );
	
	// read lines from file
	survival_vectors_per_erasure = compute_num_vectors_per_erasure_pattern( rs_k, rs_p );
	for ( int i = 0; i < survival_vectors_per_erasure; i++ ){
		ret_val = fscanf( fd_pattern, "%s", &(permutations_strings[i*rs_m]) );
		if ( ret_val == 0 ) {
			fprintf(stderr, "%s:%d: Error reading from %s\n", __FILE__, __LINE__, filename);
			return -1;
		}
	}

	convert_binary_permutations_to_array ( rs_k, rs_p, 
											survival_vectors_per_erasure, 
											(uint8_t*)permutations_strings, 
											(uint8_t*)survival_patterns_local );		


	for ( int i = 0; i < size_of_survival_patterns; i++ ) {
		survival_patterns[i] = survival_patterns_local[i];
	}

/*
	// Init output		
	if ( (rs_k == 3) && (rs_p == 2) )	{			
		// This is small enough to be hard coded here
		// uint8_t survival_patterns_3_2 [5][4][3] = {
		// 		{{1,2,3},	{2,3,4},	{1,3,4},	{1,2,4}},	// 00001	0
		// 		{{0,2,3},	{2,3,4},	{0,3,4},	{0,2,4}},	// 00010	1
		// 		{{0,1,3},	{1,3,4},	{0,3,4},	{0,1,4}},	// 00100	2
		// 		{{0,1,2},	{1,2,4},	{0,2,4},	{0,1,4}},	// 01000	3
		// 		{{0,1,2},	{1,2,3},	{0,2,3},	{0,1,3}}	// 10000	4
		// 	};
		uint8_t permutations_4_2 [4][4+1]; // binom(((rs_k+rs_p-1) rs_k)) * (rs_k+rs_p-1)

		// read lines from file
		for ( int i = 0; i < survival_vectors_per_erasure; i++ ){
			ret_val = fscanf( fd_pattern, "%s", permutations_4_2[i] );
		}

		uint8_t survival_patterns_3_2 [5][4][3] = {0};
		convert_binary_permutations_to_array ( rs_k, rs_p, 
												survival_vectors_per_erasure, 
												(uint8_t*)permutations_4_2, 
												(uint8_t*)survival_patterns_3_2 );		
	
	
		for ( int i = 0; i < sizeof(survival_patterns_3_2); i++ ) {
			survival_patterns[i] = ((uint8_t*)survival_patterns_3_2)[i];
		}
	}
	if ( (rs_k == 6) && (rs_p == 3) )	{	
		uint8_t permutations_8_3 [28][8+1]; // binom(((rs_k+rs_p-1) rs_k)) * (rs_k+rs_p-1)

		// read lines from file
		for ( int i = 0; i < survival_vectors_per_erasure; i++ ){
			ret_val = fscanf( fd_pattern, "%s", permutations_8_3[i] );
		}

		uint8_t survival_patterns_6_3 [9][28][6] = {0};
		convert_binary_permutations_to_array ( rs_k, rs_p, 
												survival_vectors_per_erasure, 
												(uint8_t*)permutations_8_3, 
												(uint8_t*)survival_patterns_6_3 );		
	
		for ( int i = 0; i < sizeof(survival_patterns_6_3); i++ ) {
			survival_patterns[i] = ((uint8_t*)survival_patterns_6_3)[i];
		}
	}	
	if ( (rs_k == 10) && (rs_p == 4) )	{	
		uint8_t permutations_13_4 [286][13+1]; // binom(((rs_k+rs_p-1) rs_k)) * (rs_k+rs_p-1)

		for ( int i = 0; i < survival_vectors_per_erasure; i++ ){
			ret_val = fscanf( fd_pattern, "%s", permutations_13_4[i] );
		}

		uint8_t survival_patterns_10_4 [14][286][10] = {0};
		convert_binary_permutations_to_array ( rs_k, rs_p, 
												survival_vectors_per_erasure, 
												(uint8_t*)permutations_13_4, 
												(uint8_t*)survival_patterns_10_4 );		

		for ( int i = 0; i < sizeof(survival_patterns_10_4); i++ ) {
			survival_patterns[i] = ((uint8_t*)survival_patterns_10_4)[i];
		}
	
	}

*/

    fclose(fd_pattern);

	return 0;
}
