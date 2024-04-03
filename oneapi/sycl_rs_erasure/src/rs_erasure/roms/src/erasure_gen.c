#include <bits/stdc++.h>
using namespace std;

int main(int argc, char** argv) {

	if ( argc < 3 ) {
		cerr << "Missing arguments:\n";
		cerr << "Usage: " << argv[0] << " K P\n";
		return -1;
	}

	// Generate the permutations from sum(i=1:loops,binom(total, i))
	int total = atoi(argv[1]);
	int loops = atoi(argv[2]);

	// Allocate char array
	char* char_array;
	char_array = (char*)malloc(total + 1);
	
	for ( int j = loops-1; j >= 0; j-- ) {
		int threshold = loops - j;

		// Generate base permutation
		for ( int i = 0; i < threshold; i++ ){
			char_array[i] = '1';
		}
		for ( int i = threshold; i < total; i++ ){
			char_array[i] = '0';
		}

		// Generate other permutations
		string s = string(char_array);
		sort(s.begin(), s.end());
		do {
			cout << s << endl;
		} while (next_permutation(s.begin(), s.end()));
	}

	return 0;
}
