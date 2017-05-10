#include <pari/pari.h> /* Include PARI headers */

#include <string>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <openssl/sha.h>
#include <openssl/bn.h>
#include <errno.h>


#include <omp.h>       /* Include OpenMP headers */

#define MAXTHREADS 3  /* Max number of parallel threads */

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


int main(void)
{
    GEN M,N1,N2, F1,F2,D,ss,prim;
    struct pari_thread pth[MAXTHREADS];
    int numth = omp_get_max_threads(), i;

    // preparing

    FILE* prime_f = NULL;
    int is_ok = -1;
    char current_prime[1024] = {0};
    char order_n[] = "199";//"0x00FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF27E69532F48D89116FF22B8D4E0560609B4B38ABFAD2B85DCACDB1411F10B275";
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

    /* Initialise the main PARI stack and global objects (gen_0, etc.) */
    pari_init(4000000000,50000000);
    if (numth > MAXTHREADS)
    {
        numth = MAXTHREADS;
        omp_set_num_threads(numth);
    }

    for (i = 1; i < numth; i++) {
        pari_thread_alloc(&pth[i],400000000,NULL);
    }

    GEN prime = strtoi(prime_line.c_str());

    clock_t start = clock();
#pragma omp parallel
        {
            int this_th = omp_get_thread_num();
            if (this_th) (void) pari_thread_start(&pth[this_th]);

            while(is_ok != 0) {
//#pragma omp for
                for (int ii = 0; ii < numth; ++ii) {
                    //pari_sp av = avma; /* record initial avma */
                    GEN t = gdivent(glog(prime, 5), glog(gp_read_str("2"), 5));
                    int exp = (int) itos(t);
                    int s = (exp - 1) / 160;
                    int v = exp - 160 * s;

                    srandom(time(NULL));
                    // so now we have g == 176 (22 * 8 bit)
                    unsigned g = 176;
                    unsigned char seedE[22] = {0};
                    unsigned char sha_H[20] = {0};
                    for (int i = 0; i < 22; ++i) {
                        seedE[i] = (unsigned char) (rand() * (ii + 12));
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
                    // convert hex to decimal string
                    BIGNUM *big_r = BN_new();
                    BN_hex2bn(&big_r, (const char *)result_r + 2);
                    char* r_dec = BN_bn2dec(big_r);
                    //printf("th=%d r=%s\n", omp_get_thread_num(), r_dec);
                    BN_free(big_r);

                    GEN r = strtoi(r_dec);
                    OPENSSL_free(r_dec);
                    //pari_printf("r=%Ps\n", r);

                    if (cmpii(r, gp_read_str("0")) == 0 || cmpii(gmod(addmulii(gp_read_str("27"), gp_read_str("4"), r), prime), gp_read_str("0")) == 0) {
                        is_ok = -1;
                        //avma = av;
                        continue;
                    }

                    GEN n = strtoi(order_n);

                    GEN cardinality = Fp_ellcard_SEA(r, r, prime, 0);
                    //pari_printf("card=%Ps\n", cardinality);
                    //pari_printf("n=%Ps\n", n);
                    if (cmpii(gmod(cardinality, n), gp_read_str("0")) != 0) {
                        //printf("n does not divide the cardinality!\n");
                        is_ok = -1;
                        //avma = av;
                        continue;
                    }


                    for (int k = 1; k < 21; ++k) {
                        int is_error = 0;
                        if (cmpii(gmod(gsub(gpow(prime, gp_read_str(to_string(k).c_str()), 5), gp_read_str("1")), n), gp_read_str("0")) == 0) {
                            //printf("n devides p^%d - 1!\n", k);
                            is_error = 1;
                            break;
                        }
                        if (is_error) {
                            is_ok = -1;
                            continue;
                        }
                    }

                    if (cmpii(prime, n) == 0) {
                        printf("Prime p equals n !\n");
                        is_ok = -1;
                        //avma = av;
                        continue;
                    }
                    is_ok = 0;
                    clock_t end = clock();
                    printf("from ii = %d\n",ii);
                    printf("Time = %f\n", (float) (end - start) / CLOCKS_PER_SEC);
                    pari_printf("cardinality = %Ps\n", cardinality);
                    pari_printf("a,b = %Ps\n", r);
                    pari_printf("p = %Ps\n", prime);
                    pari_printf("n = %Ps\n", n);

                    FILE* f = fopen("file_parallel199.log", "a+");
                    //pari_fprintf(f, "r = %Ps\n", r);
                    //pari_fprintf(f, "p = %Ps\n", prime);
                    //pari_fprintf(f, "card = %Ps\n", cardinality);
                    //pari_fprintf(f, "n = %Ps\n", n);
                    fprintf(f, "Time = %f\n", (float)(end - start) / CLOCKS_PER_SEC);
                    fclose(f);

                    //avma = av;
                }
        }
            if (this_th) {
                pari_thread_close();
            }

        for (i = 1; i < numth; i++) {
            pari_thread_free(&pth[i]);
        }
    }/* omp parallel */

    return 0;
}
