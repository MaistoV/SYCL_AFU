#include <bits/stdc++.h>
using namespace std;

int main(int argc, char** argv) {

	if ( argc < 3 ) {
		cerr << "Missing arguments:\n";
		cerr << "Usage: " << argv[0] << " K P\n";
		return -1;
	}

	int rs_k = atoi(argv[1]);
	int rs_p = atoi(argv[2]);

	// Allocate char array
	char* char_array;
	char_array = (char*)malloc(rs_k + rs_p + 1);
	
	// Generate base permutation
	for ( int i = 0; i < rs_k; i++ ){
		char_array[i] = '1';
	}
	for ( int i = rs_k; i < rs_k+rs_p-1; i++ ){
		char_array[i] = '0';
	}

	// Generate other permutations
	string s = string(char_array);
	sort(s.begin(), s.end());
	do {
		cout << s << endl;
	} while (next_permutation(s.begin(), s.end()));

	return 0;
}
