#include <stdio.h>
#include "collatz.h"
#ifdef DYNAMIC
#include <dlfcn.h>
#endif

void test(int input, int length, int steps[])
{
    printf("\n Wejście: %d\n Kroki do 1: ",input);
    for (int i = 0; i < length; i++)
        printf("%d ", steps[i]);
    printf("\nIlość kroków: %d\n\n",length);
}

#ifdef DYNAMIC
int main()
{
    void *handle = dlopen("./libsharedlib.so", RTLD_LAZY);
    if (!handle) {
        printf("Błąd otwierania biblioteki!\n");
        return 0;
    }

    int (*collatz_test)(int, int, int*) = dlsym(handle, "test_collatz_convergence");
    if (!collatz_test) {
        printf("Błąd funkcji!\n");
        return 0;
    }

    int input = 2000;
    int max_iter = 150;
    int steps[max_iter];
    int length = collatz_test(input, max_iter, steps);
    test(input, length, steps);
    dlclose(handle);
    return 0;
}
#else
int main()
{
    int input = 2000;
    int max_iter = 150;
    int steps[max_iter];
    int length = test_collatz_convergence(input, max_iter, steps);
    test(input, length, steps);
    return 0;
}
#endif

