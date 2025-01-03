#include <stdio.h>


#if 0
#include "blis.h"
#define SIZE 4
int main() {
    float a[SIZE] = {1, 2, 3, 4};
    float b[SIZE] = {1, 2, 3, 4};

    bli_saddv(BLIS_NO_CONJUGATE, SIZE, a, 1, b, 1);
    return 0;
}
#else
int main() {
    asm("xexample.subincacc a0, a1, a1");
    printf("Hello %f\n", 8.0);

}
#endif