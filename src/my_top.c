/* Author(s): Gabriel Chiquetto & Leonardo Rocha
 * Manage my_top modules, including the interface and the process manager.
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>


#define NO_ARGS              ""
#define TEXT_INTERFACE_FILE  "./build/text_interface" 
#define PROCESS_MANAGER_FILE "./build/process_manager"
#define MAP_ANONYMOUS	     0x20
#define NO_ROOT_FILE         -1
#define NO_OFFSET            0 
#define SHARED_MEM_SIZE      63489
#define SHARED_MEM_KEY       42
#define KILL_FLAG            0
#define BUFFER_SIZE          200
#define SHARED_MEM_FLAGS     S_IRWXG | S_IRWXU | S_ISUID | IPC_CREAT | IPC_EXCL

char error_buffer[BUFFER_SIZE];

char* create_shared_memory();

char* create_SYSTEMV_shared_memory();

int main() 
{
    pid_t pid_text_interface, pid_process_manager;
    char * args[3];
    args[2] = NULL;
    int error = 0;

    char *shared_memory = create_SYSTEMV_shared_memory();

    /*
    char shared_address[16];
    sprintf(shared_address, "%lld",(long long) shared);    
    args[1] = shared_address;
    */
    memset(shared_memory, 'A', 100);

    pid_process_manager = fork();
    if(pid_process_manager == 0) {
        args[0] = PROCESS_MANAGER_FILE;
        error = execvp(PROCESS_MANAGER_FILE, args);
        if(error == -1) 
        {   
            snprintf(error_buffer, BUFFER_SIZE, "Error in execvp(PROCESS_MANAGER_FILE)");
            perror(error_buffer);
        }
    }

    pid_text_interface = fork();
    if (pid_text_interface == 0)
    {
        args[0] = TEXT_INTERFACE_FILE;
        error = execvp(TEXT_INTERFACE_FILE, args);
        if(error == -1) 
        {   
            snprintf(error_buffer, BUFFER_SIZE, "Error in execvp(TEXT_INTERFACE_FILE)");
            perror(error_buffer);
        }
    }

    //TODO: if Flags used change, change this. 
   
    sleep(3);
    //while(shared[KILL_FLAG] != 1);
        
    kill(pid_process_manager, SIGKILL);

    /* detach from the segment: */
    if (shmdt(shared_memory) == -1) {
        perror("shmdt");
        exit(1);
    }

    return 0;
}

/*
 * Function:  create_shared_memory
 * --------------------
 * Creates a rw shared memory using the function mmap.
 * it can be used by this process and its children.
 *  size: size of the shared memory created.
 */
char* create_shared_memory() {

    long size = 10 * sysconf(_SC_PAGE_SIZE);

    int protection = PROT_READ | PROT_WRITE;

    // The buffer will be shared (meaning other processes can access it), but
    // anonymous,  only this process and its children will be able to use it
    int visibility = MAP_SHARED | MAP_ANONYMOUS;

    return mmap(NULL, size, protection, visibility, NO_ROOT_FILE, NO_OFFSET);
}

char* create_SYSTEMV_shared_memory() 
{   
    int shmid;
    char *data;
    
    /*  create the segment: */
    if ((shmid = shmget(SHARED_MEM_KEY, 128, 0660 | IPC_CREAT)) == -1) {
        perror("shmget");
        exit(1);
    }

     /* attach to the segment to get a pointer to it: */
    data = shmat(shmid, NULL, 0);
    if (data == (char *)(-1)) {
        perror("shmat");
        shmctl(shmid, IPC_RMID, NULL);
        exit(1);
    }

    return data;
}
