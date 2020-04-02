/* Author(s): Gabriel Chiquetto & Leonardo Rocha
 * Manage my_top modules, including the interface and the process manager.
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <signal.h>
#include <errno.h>

#define NO_ARGS              ""
#define TEXT_INTERFACE_FILE  "./build/text_interface" 
#define PROCESS_MANAGER_FILE "./build/process_manager"
#define MAP_ANONYMOUS	     0x20
#define NO_ROOT_FILE         -1
#define SHARED_MEM_SIZE      63489
#define KILL_FLAG            0
#define BUFFER_SIZE          200


char error_buffer[BUFFER_SIZE];

int main() 
{
    pid_t pid_text_interface, pid_process_manager;
    char * args[3];
    args[2] = NULL;
    int error = 0;
    // Maps an allocated memory space, not backed by any file, for shared communication between processes. 
    char *shared = mmap(0, SHARED_MEM_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, NO_ROOT_FILE, 0);

    char shared_address[16];
    sprintf(shared_address, "%lld",(long long) shared);    
    args[1] = shared_address;

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
    while(shared[KILL_FLAG] != 1) 
        printf("%d", shared[0]);
        
    kill(pid_process_manager, SIGKILL);

    return 0;
}