#include<pari/pari.h>
#include <string>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <openssl/sha.h>
#include <errno.h>

#include <time.h>


using namespace std;
// for adding array which stands for number and number b
void add_to_arr(unsigned char* arr, int length, unsigned char b) {

    int carry = 0;
    if((int)arr[0] + (int)b > 255) {
        carry = 1;
    }
    arr[0] = arr[0] + b;
    for(int i = 1; i < length; ++i) {
        if((int)arr[i] + carry > 255) {
            arr[i] = arr[i] + carry;
            carry = 1;
        } else {
            arr[i] = arr[i] + carry;
            carry = 0;
        }
    }

}

int main() {
    FILE* prime_f = NULL;
    int is_ok = -1;
    char current_prime[1024] = {0};
    char order_n[] = "503";//"0x196461B";
            //"0x00FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF27E69532F48D89116FF22B8D4E0560609B4B38ABFAD2B85DCACDB1411F10B275";
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
    pari_init(100000000000, 10000000000);
    GEN r;
// creating prime object with string of prime number converted to C string
 //   GEN prime = gp_read_str(prime_line.c_str());
    GEN prime = strtoi(prime_line.c_str());
// void pari_fprintf(FILE *file, const char *fmt, ...) for logging to a file
#ifdef PRINT_INFO
    pari_printf("%Ps\n", prime);
#endif
    GEN cardinality;
    GEN n;

    clock_t start = clock();
    pari_sp av = avma; /* record initial avma */
    while(is_ok != 0) {
        avma = av;
        // compute floor(log2(prime)); stoi(2) is required to convert int 2 to GEN object
        GEN t = gdivent(glog(prime, 5), glog(stoi(2), 5));
        //pari_printf("%Ps\n", t);

        // convert GEN t to long exp because t is not more than 512
        int exp = (int) itos(t);
        // this all is according to algorithm
        int s = (exp - 1) / 160;
        int v = exp - 160 * s;

        srandom(time(NULL));

        // so now we have g == 176 (22 * 8 bit)
        unsigned g = 176;
        unsigned char seedE[22] = {0};
        unsigned char sha_H[20] = {0};
        for (int i = 0; i < 22; ++i) {
            seedE[i] = (unsigned char) rand();
        }
        while (seedE[20] == 0) {
            seedE[20] = (unsigned char) rand();
        }
        SHA1(seedE, 22, sha_H);

        int full_chars = v / 8;
        int part_char = v % 8;
        if (part_char != 0) {
            sha_H[full_chars] = (sha_H[full_chars] << (8 - part_char)) >> (8 - part_char);
        }

        // it is not required; probably should be commented
        for (int i = full_chars + 1; i < 22; ++i) {
            sha_H[i] = 0;
        }

        // replace the leftest significant bit in the result
        if (part_char != 0) {
            sha_H[full_chars] = (sha_H[full_chars] << (8 - part_char + 1)) >> (8 - part_char + 1);
        } else {
            sha_H[full_chars - 1] = sha_H[full_chars - 1] & 0x7f;
        }

        char result_r[20 * 4 * 2 + 3] = {0};
        result_r[0] = '0';
        result_r[1] = 'x';
#ifdef PRINT_INFO
        printf("fullchars = %d\n", full_chars);
#endif
        for (int i = full_chars; i >= 0; --i) {
            // writes in buffer string hex + 00 !!!
            sprintf(&result_r[2 + (full_chars - i) * 2], "%02X", sha_H[i]);
        }

        unsigned char WW[20 * 3] = {0};
        unsigned char sha_WW[20] = {0};
        for (int j = 1; j <= s; ++j) {
            add_to_arr(sha_H, 20, 1);
            SHA1(sha_H, 20, sha_WW);

            for (int i = 19; i >= 0; --i) {
                // 2 + (full_chars + 1) * 2 + 40 * (j - 1) chars are already filled
                sprintf(&result_r[2 + (full_chars + 1) * 2 + 40 * (j - 1) + (19 - i) * 2], "%02X", sha_WW[i]);
            }
        }
#ifdef PRINT_INFO
        printf("%s\n", result_r);
#endif
        r = gmod(strtoi(result_r), prime);
#ifdef PRINT_INFO
        pari_printf("r = %Ps\n", r);
        pari_printf("p = %Ps\n", prime);
#endif

        if (mpcmp(r, stoi(0)) == 0 || mpcmp(gmod(addmulii(stoi(27), stoi(4), r), prime), stoi(0)) == 0) {
            is_ok = -1;
            continue;
        }
        n = strtoi(order_n);

        cardinality = Fp_ellcard_SEA(r, r, prime, 0);
#ifdef PRINT_INFO
        pari_printf("cardinality = %Ps\n", cardinality);
#endif

        if (mpcmp(gmod(cardinality, n), stoi(0)) != 0) {
#ifdef PRINT_INFO
            printf("n does not divide the cardinality!\n");
#endif
            is_ok = -1;
            continue;
        }
        int is_error = 0;
        for (int k = 1; k < 21; ++k) {
            is_error = 0;
            if (mpcmp(gmod(gsub(gpow(prime, stoi(k), 5), stoi(1)), n), stoi(0)) == 0) {
                printf("n devides p^%d - 1!\n", k);
                is_error = 1;
                break;
            }
        }
        if(is_error) {
            continue;
        }

        if (mpcmp(prime, n) == 0) {
            printf("Prime p equals n !\n");
            is_ok = -1;
            continue;
        }
        is_ok = 0;
    }
    clock_t end = clock();
    FILE* f = fopen("file503.log", "a+");

    pari_printf("r = %Ps\n", r);
    pari_printf("p = %Ps\n", prime);
    pari_printf("card = %Ps\n", cardinality);
    pari_printf("n = %Ps\n", n);


    //pari_fprintf(f, "r = %Ps\n", r);
    //pari_fprintf(f, "p = %Ps\n", prime);
    //pari_fprintf(f, "card = %Ps\n", cardinality);
    //pari_fprintf(f, "n = %Ps\n", n);
    fprintf(f, "Time = %f\n", (float)(end - start) / CLOCKS_PER_SEC);
    printf("Time = %f\n", (float)(end - start) / CLOCKS_PER_SEC);
    fclose(f);

    return 0;
}
