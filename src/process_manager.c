#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>

#define SHARED_MEM_SIZE      63489
#define SHARED_MEM_KEY       0x1235
#define SHARED_MEM_FLAGS     0660 | IPC_CREAT

char* shared_memory;

char* get_shared_memory();

int main(int argc, char *argv[]) 
{
    //shared = (char*) atoll(argv[1]);   
    shared_memory = get_shared_memory();

    printf("Entrou na PM %c\n", shared_memory[0]);

    /* detach from the segment: */
    if (shmdt(shared_memory) == -1) {
        perror("shmdt");
        exit(1);
    }

    return 0;
}

char* get_shared_memory() 
{   
    int shmid;
    char *data;
    
    /*  create the segment: */
    if ((shmid = shmget(SHARED_MEM_KEY, 128, SHARED_MEM_FLAGS)) == -1) {
        perror("shmget");
        exit(1);
    }

     /* attach to the segment to get a pointer to it: */
    data = shmat(shmid, NULL, 0);
    if (data == (char *)(-1)) {
        perror("shmat");
        exit(1);
    }

    return data;
}
