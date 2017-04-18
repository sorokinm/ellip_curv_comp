#include<pari/pari.h>
#include <stdio.h>

using namespace std;

int main(){

 char str [618] = {0};
 scanf("%s",str);
 pari_init(10000000, 1000000);
 pari_sp av = avma;
 avma = av;
 GEN a = gp_read_str(str);
 pari_printf("%Ps", a);


 return 0;

}
