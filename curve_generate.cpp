#include<pari/pari.h>
#include <string>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <openssl/sha.h>

using namespace std;

int main() {
    FILE* prime_f = NULL;
    char current_prime[200] = {0};
    string prime_line;
    ifstream infile ("primes");

    if (infile) {
        getline(infile, prime_line);
        //cout << prime_line << endl;
    } else {
        cout << "Can't open file with modulo!!!" << endl;
        cout << "To create it use openssl prime -generate -bits 512 >> primes" << endl;
        return -1;
    }
    infile.close();
// initialisation of pari system
    pari_init(10000000, 1000000);

// creating prime object with string of prime number converted to C string
    GEN prime = gp_read_str(prime_line.c_str());
// void pari_fprintf(FILE *file, const char *fmt, ...) for logging to a file
//pari_printf("%Ps\n", prime);

    // compute floor(log2(prime)); stoi(2) is required to convert int 2 to GEN object
    GEN t = gdivent(glog(prime, 5), glog(stoi(2), 5));
    pari_printf("%Ps\n", t);

    // convert GEN t to long exp because t is not more than 512
    int exp = (int)itos(t);
    // this all is according to algorithm
    int s = (exp - 1) / 160;
    int v = exp - 160 * s;

    srandom(time(NULL));

    // so now we have g == 176 (22 * 8 bit)
    unsigned char g[22] = {0};
    unsigned char sha_H[20] = {0};
    for(int i = 0; i < 22; ++i) {
        g[i] = (unsigned char) rand();
    }
    while(g[21] == 0) {
        g[21] = (unsigned char) rand();
    }
    SHA1(g, 22, sha_H);

    int full_chars = v / 8;
    int part_char = v % 8;
    if(part_char != 0) {
        sha_H[full_chars] = (sha_H[full_chars] << (8 - part_char)) >> (8 - part_char);
    }
    // it is not required; probably should be commented
    for(int i = full_chars + 1; i < 22; ++i) {
        sha_H[i] = 0;
    }

    // replace the leftest significant bit in result
    if(part_char != 0) {
        sha_H[full_chars] = (sha_H[full_chars] << (8 - part_char + 1) ) >> (8 - part_char + 1);
    } else {
        sha_H[full_chars - 1] = sha_H[full_chars - 1] &  0x7f;
    }


    return 0;
}
