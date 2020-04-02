#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) 
{
    char *shared = (char*) atoll(argv[1]);

    shared[1] = 'c';
    shared[0] = 1;
    return 0;
}