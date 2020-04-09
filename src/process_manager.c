#include <stdio.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/stat.h>

#define PROC_DIR "/proc"
#define STAT_DIR "/stat"
#define PROC_STAT_PATH "/proc/stat"
#define TRASH_SIZE 10
#define BUFFER_SIZE 200

typedef struct
{
   long unsigned int user_time;
   long unsigned int kernel_time;
   long long unsigned int start_time;
} time_info;

typedef struct 
{
    pid_t pid;
    time_info previous_time;    
} procs_time_samples;

typedef struct  process_info{
    pid_t pid;
    char user_name[32];
    char state;
    int priority;
    int nice;
    float cpu_percentage;
    time_info time;
    unsigned cpu_time;
    char command[64];
} process_info;

char error_buffer[BUFFER_SIZE];

process_info** proc_table_generator(int *num_valid_procs);
process_info * read_proc_stat(const char * stat_file);
char * get_stat_file_path(char * entry_name, int proc_dir_length, int stat_dir_length);
int handle_file_open(FILE **file_stream, const char* mode, const char *file_name);
void clear_table(process_info ** process, int table_size);

/* This method returns a list of process_info*/
int main(int argc, char *argv[]) 
{   
    process_info** process_info_table;
    int num_valid_procs;
    
    /* the identifier for the shared memory segment */
	int segment_id;
	/* a pointer to the shared memory segment */
	char* shared_memory;

    if(argc != 2) {
        printf("Usage: ./process_manager <segment_id>");
        exit(-1);
    }

	segment_id = atoi(argv[1]);
    //printf("\nSegment ID in process manager : %d\n", segment_id);

	/** attach the shared memory segment */
	shared_memory = (char *) shmat(segment_id, NULL, 0);
	//printf("shared memory segment %d attached at address %p\n", segment_id, shared_memory);

    while(1)
    {
        num_valid_procs = 0;
        process_info_table = proc_table_generator(&num_valid_procs);
        clear_table(process_info_table, num_valid_procs);
        sleep(1);
        // pegar informações summary no /proc/stat 
        // normal_process_user %s
        // niced_process_user %s
        // process_kernel %s
        // procs_running %s
    }

    /** now detach the shared memory segment */ 
	if ( shmdt(shared_memory) == -1) 
    {
		fprintf(stderr, "Unable to detach\n");
	}

    return 0;
}

process_info** proc_table_generator(int *num_valid_procs)
{
    DIR *directory;
    struct dirent *directory_entry;
    int proc_dir_length = strlen(PROC_DIR);
    int stat_dir_length = strlen(STAT_DIR);
    char* stat_file_path;
    process_info **process_info_table = (process_info **) malloc(sizeof(process_info*));
    process_info *proc_info;
    //printf("PID\tUSER\tPR\tNI\tS\t%%CPU\tTIME+\tCOMMAND\t\t ENTRY\n");
    
    if ((directory = opendir (PROC_DIR)) != NULL) 
    {
        while ((directory_entry = readdir (directory)) != NULL) 
        {   
            // if starts with a digit, it's a PID
            if(isdigit(directory_entry->d_name[0])) 
            {   
                stat_file_path = get_stat_file_path(directory_entry->d_name, proc_dir_length, stat_dir_length);
                proc_info = read_proc_stat(stat_file_path);
                if (proc_info != NULL) 
                { 
                    process_info_table = (process_info **) realloc(process_info_table, ++(*num_valid_procs) * sizeof(process_info*));
                    process_info_table[*num_valid_procs - 1] = proc_info;
                    // printf("%d\tUSER\t%d\t%d\t%c\t%.2f\t%lu\t%s\t\t%d\n", proc_info->pid, proc_info->priority, proc_info->nice,
                    //     proc_info->state, 0.0, proc_info->time.user_time, proc_info->command, num_valid_procs -1 );
                }
                free(stat_file_path); 
            }
        }
        process_info_table = (process_info **) realloc(process_info_table, ++(*num_valid_procs) * sizeof(process_info*));
        // marks the end of the list
        process_info_table[*num_valid_procs - 1] = NULL;

        closedir(directory);
        return process_info_table;
    } 
    else 
    {
        /* could not open directory */
        perror ("Error in process_manager opendir(\"./proc\")");
        free(process_info_table);
        return -1;     
    }
}

char * get_stat_file_path(char * entry_name, int proc_dir_length, int stat_dir_length)
{
    int directory_entry_length = strlen(entry_name);
    int proc_path_length = proc_dir_length + stat_dir_length + directory_entry_length + 2; // 2 for "/" and '\0'
    char *stat_file_path = (char *) calloc(proc_path_length, sizeof(char));
    sprintf(stat_file_path, "%s/%s%s", PROC_DIR, entry_name, STAT_DIR);

    return stat_file_path;
}

process_info * read_proc_stat(const char * stat_file)
{
    FILE * stat_filestream;
    int handle_file_return = handle_file_open(&stat_filestream, "r", stat_file);

    if(handle_file_return != 0) 
    {
        fclose(stat_filestream);
        return NULL;
    }
        
    process_info* process = (process_info *) malloc(sizeof(process_info));
    long unsigned int lixo;
    char whitespace_buf;
    fscanf(stat_filestream, "%d%s%c%c", &(process->pid), process->command, &whitespace_buf,&process->state);
    //This Process is dead, move on
    if(process->state == 'X')
    {
        fclose(stat_filestream);
        free(process);
        return NULL;
    }   
    else
    {
        for(int i = 0; i < TRASH_SIZE; i++)  fscanf(stat_filestream ,"%lu", &lixo);
        fscanf(stat_filestream ,"%lu%lu", &(process->time.user_time), &(process->time.kernel_time));
        fscanf(stat_filestream, "%*ld%*ld");
        fscanf(stat_filestream, "%d%d"  , &(process->priority), &(process->nice));
        fscanf(stat_filestream, "%*ld%*ld");
        fscanf(stat_filestream, "%llu",   &(process->time.start_time)); 
    }
    fclose(stat_filestream);
    return process;
} 

/*
 * Function:  handle_file_open 
 * --------------------
 * Tries to open the given file and handle errors
 * 
 *  file_stream: pointer to assign the file_stream if the file has been opened
 *	file_name: path for the file to be open	
 *	mode: file open mode (e.g. - r, w, rb...)
 * 
 *  returns: zero if the file was opened succesfully
 *           returns -1 on error
 */
int handle_file_open(FILE **file_stream, const char* mode, const char *file_name) 
{	
	if(file_stream != NULL) 
	{
		*file_stream = fopen(file_name, mode);
		if (*file_stream == NULL) 
		{
			snprintf(error_buffer, BUFFER_SIZE, "Could not open file \"%s\"", file_name);
			perror(error_buffer);
			return -1;
		}
	}
			
	return 0;
}

void clear_table(process_info ** process, int table_size)
{
    for(int i = table_size - 1; i >= 0; i--)
    {   
        free(process[i]);
    }
    free(process);
}

long long unsigned cpu_total_time()
{
    FILE* proc_stat_filestream;
    long long unsigned user, nice, system, idle, total_time;
    
    int file_open_return = handle_file_open(proc_stat_filestream, "r", PROC_STAT_PATH);
    
    if(file_open_return == -1)
        return -1;
    fscanf(proc_stat_filestream, "%s", line);
    sscanf(line,"%*s %llu %llu %llu %llu", &user, &nice, &system, &idle);
    total_time = user + nice + system + idle;    
    return total_time;
}

float update_cpu_usage(process_info** process_table, int num_procs)
{   
    char line[256];
    static time_info time_before = {0,0,0,0,0}; 
    static time_info time_after  = {0,0,0,0,0};
    static long long unsigned time_total_before = 0; 
    static long long unsigned time_total_after  = 0;
    
    time_before = time_after;
    time_after = current_proc_time;
    
    time_total_before = time_total_after;
    time_total_after = cpu_total_time();

    for(int i = num_procs; i >= 0; i--)    
        calculate_cpu_usage(time_before, time_after, time_total_before, time_total_after);
}

float calculate_cpu_usage(time_info time_before, time_info time_after,
    long long unsigned time_total_before, long long unsigned time_total_after)
{   
    long long unsigned delta_total_time = time_total_after - time_total_before;
    long long unsigned total_time_after = time_after.user_time + time_after.kernel_time;
    long long unsigned total_time_before = time_before.user_time + time_before.kernel_time;
    float total_time = 100 * (total_time_after - total_time_before)  / delta_total_time;

    return total_time;
}
