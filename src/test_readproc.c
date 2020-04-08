#include <stdio.h>
#include "readproc.h"

int main() 
{   
    openproc(PROC_FILLUSR);    //proc_t** procs = readproctabs3(PROC_FILLUSR | PROC_FILLSTAT | PROC_FILLSTATUS);
    //printf("procs[0]->priority %d", procs[0]->priority);

    return 0;
}
