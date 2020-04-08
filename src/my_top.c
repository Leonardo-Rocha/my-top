/* Author(s): Gabriel Chiquetto & Leonardo Rocha
 * Manage my_top modules, including the interface and the process manager.
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/signal.h>

#define TEXT_INTERFACE_FILE  "./build/text_interface" 
#define PROCESS_MANAGER_FILE "./build/process_manager"
#define SEGMENT_SIZE 32768

#define BUFFER_SIZE 600

char error_buffer[BUFFER_SIZE];

int main() 
{
    pid_t pid_text_interface, pid_process_manager;
    char * args[3];
    int error = 0;
	int segment_id;
	char* shared_memory;

    args[1] = (char *) calloc(16, sizeof(char));
    // sets an end to the args
    args[2] = NULL; 

	/** allocate  a shared memory segment */
	segment_id = shmget(IPC_PRIVATE, SEGMENT_SIZE, S_IRUSR | S_IWUSR);

	/** attach the shared memory segment */
	shared_memory = (char *) shmat(segment_id, NULL, 0);
	//printf("shared memory segment %d attached at address %p\n", segment_id, shared_memory);

	sprintf(args[1],"%d", segment_id);

	pid_text_interface = fork();
	if(pid_text_interface == 0) 
        exec_module(TEXT_INTERFACE_FILE, args);
    else 
    {   
        pid_process_manager = fork();
        if(pid_text_interface == 0) 
        {
            exec_module(PROCESS_MANAGER_FILE, args);
        }           

        waitpid(pid_text_interface, NULL, 0);
        kill(PROCESS_MANAGER_FILE, SIGKILL);

        /** now detach the shared memory segment */ 
        if (shmdt(shared_memory) == -1) 
        {
            fprintf(stderr, "Unable to detach\n");
        }

        /** now remove the shared memory segment */
        shmctl(segment_id, IPC_RMID, NULL); 
    }
	
	return 0;
}

void exec_module(const char *file_path, char* args[3])
{
    args[0] = file_path;

    int error = execvp(file_path, args);
    if(error == -1)
    { 
        snprintf(error_buffer, BUFFER_SIZE, "Error in execvp(%s)", file_path);
        perror(error_buffer);
    }
}