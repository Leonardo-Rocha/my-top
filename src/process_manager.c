#include <stdio.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <errno.h>
#include "hashtable.h"
#include <unistd.h>
#include <pwd.h>

#define PROC_DIR          "/proc"
#define STAT_DIR          "/stat"
#define PROC_STAT_PATH    "/proc/stat"
#define PROC_CPUINFO_FILE "/proc/cpuinfo"
#define OUTPUT_FILE_PATH  "output.txt"
#define TRASH_SIZE        10 
#define BUFFER_SIZE       200
#define ACCESS_POINT_MAX  100
#define HASH_TABLE_MAX    600
#define COMMAND_MAX       64
#define USER_NAME_MAX     10

typedef struct
{
   long unsigned int user_time;
   long unsigned int kernel_time;
} time_info;

typedef struct 
{
    // the moment where it was accessed
    unsigned short access_point; 
    time_info proc_time_info;
} hash_data;

typedef struct 
{
    unsigned int valid_counter;
    unsigned int running_counter;
    unsigned int sleeping_counter;
    unsigned int stopped_counter;
    unsigned int zombie_counter;
} task_counter;

typedef struct process_info
{
    pid_t pid;
    char *user_name; // this is allocated with size USER_NAME_MAX
    char state;
    int priority;
    int nice;
    float cpu_percentage; 
    time_info time;
    unsigned cpu_time;
    char *command; // this is allocated with size COMMAND_MAX
} process_info;

char error_buffer[BUFFER_SIZE];

/* This method returns a list of process_info and updates the task counter. */
process_info** proc_table_generator(task_counter *tasks, unsigned short access_point);

/* Reads the stat of a single process and return it if was successful. Returns null otherwise. */
process_info* read_proc_stat(const char * stat_file);

/* Reads the username of a proc by searching for the uid in /status and writes it in proc_info. */
void read_proc_username(process_info* proc_info, char *stat_file);

/* Returns the uid by parsing the /proc/[pid]/status file. */
uid_t read_proc_uid(char * stat_file);

/* Returns the stat file path concatenating PROC_DIR with the given pid and STAT_DIR. */
char* get_stat_file_path(char* pid, int proc_dir_length, int stat_dir_length);

/* Frees all entries of the process_info_table and the table itself. */
void clear_process_info_table(process_info ** process_info_table, int table_size);

/* Updates the state counters of the tasks like sleeping, running, etc. */
void update_state_counters(char state, task_counter* tasks);

/* Updates the cpu time and cpu percentage of the given process. */
void proc_cpu_update(process_info* proc_info, long long unsigned cpu_time);

/* Returns the cpu usage of a process using a sample of time and doing current - previous. */
float get_cpu_usage(time_info proc_time_previous, time_info proc_time_current, long long unsigned cpu_time);

/* Returns the num of cpus to calculate cpu percentage. */
unsigned int get_num_cpus();

/* Returns the cpu_total_time = (user + nice + system + idle) using cpu times(first line of /proc/stat) */
long long unsigned cpu_total_time();

/* Updates the hash table with the process time info and the cpu info of the given process. */
void time_table_update(process_info * proc_info, unsigned short access_point, long long unsigned cpu_time);

/* Stores a new entry in the time table, where the key is the pid and data contains time_info. */
void time_table_store(process_info * proc_info, unsigned short access_point);

/* Flushes unused entries of the time table. */
void time_table_flush(hash_entry* time_table, unsigned short access_point);

/* Frees all entries of the time_table and the table itself. */
void clear_time_table(hash_entry* time_table);

/* Calls fopen and returns zero if the file was opened succesfully and -1 on error, also printing to stderr. */
int handle_file_open(FILE **file_stream, const char* mode, const char *file_name);

/* Compare two processes sorting first by CPU percentage and then by PID. */
static int compare_processes(const void * a, const void * b);

/* Initialize the task counter struct with zeros. */
void initialize_task_counters(task_counter *tasks);

/* Writes all table info into the shared memory*/
void write_info_in_memory(task_counter tasks, process_info** process_info_table, char* shared_memory);

/* Print all process with a pretty format. Prints to the stdout if write_to_file == 0. */
void print_process_info_table(process_info **proc_info_table, task_counter tasks, short write_to_file);

/* Global variable used to store number of cpu's in the system, used in the cpu usage calculation. */
unsigned int num_cpus;

int main(int argc, char *argv[]) 
{   
    process_info** process_info_table;
    task_counter tasks = {0,0,0,0,0};
    unsigned short access_point = 0;
	int memory_segment_id;
	char* shared_memory;

    if(argc != 2) {
        printf("Usage: ./process_manager <segment_id>");
        exit(-1);
    }

	memory_segment_id = atoi(argv[1]);

	/** attach the shared memory segment */
	shared_memory = (char *) shmat(memory_segment_id, NULL, 0);

    num_cpus = get_num_cpus();

    hash_entry* time_table = hash_create(HASH_TABLE_MAX);
    process_info_table = proc_table_generator(&tasks, access_point);
    clear_process_info_table(process_info_table, tasks.valid_counter);
    access_point++;
    access_point = access_point % ACCESS_POINT_MAX;

    while(1)
    {
        initialize_task_counters(&tasks); 
        sleep(1);
        process_info_table = proc_table_generator(&tasks, access_point);
        qsort(process_info_table, tasks.valid_counter - 1, sizeof(process_info*), compare_processes);
        write_info_in_memory(tasks, process_info_table, shared_memory);
        clear_process_info_table(process_info_table, tasks.valid_counter);
        time_table_flush(time_table, access_point);
        access_point++;
        access_point = access_point % ACCESS_POINT_MAX;   
   }

    /** now detach the shared memory segment */ 
	if ( shmdt(shared_memory) == -1) 
    {
		fprintf(stderr, "Unable to detach\n");
	}

    clear_time_table(time_table);

    return 0;
}

void time_table_update(process_info * proc_info, unsigned short access_point, long long unsigned cpu_time)
{
    proc_cpu_update(proc_info, cpu_time);
    time_table_store(proc_info, access_point);
}

void time_table_flush(hash_entry* time_table, unsigned short access_point)
{
    int key;
    hash_data* data;
    for(int i = 0; i < HASH_TABLE_MAX; i++)
    {
        key = time_table[i].key;
        data =  (hash_data*) time_table[i].data;
        if(key != -1 && data->access_point != access_point)
        {
            hash_delete(key);
            free(data);
        }
    }
}

void proc_cpu_update(process_info* proc_info, long long unsigned cpu_time)
{
    int key = proc_info->pid;
    hash_entry *proc_previous_entry = hash_find(key);
    if(proc_previous_entry != NULL)
    {   
        hash_data *proc_previous_data = (hash_data*) proc_previous_entry->data; 
        time_info current_proc_time = proc_info->time; 
        time_info previous_proc_time = proc_previous_data->proc_time_info;
    
        proc_info->cpu_time = current_proc_time.kernel_time + current_proc_time.user_time;
        proc_info->cpu_percentage = get_cpu_usage(previous_proc_time, current_proc_time, cpu_time);
    }
    else 
    {
        proc_info->cpu_time = 0; 
        proc_info->cpu_percentage = 0.0;
    }
}

void time_table_store(process_info * proc_info, unsigned short access_point)
{
    hash_data*  proc_data;
    hash_entry* entry;
    hash_entry new_entry;
    
    entry = hash_find(proc_info->pid);
    if(entry != NULL)
    {
        proc_data = (hash_data*) entry->data;
        proc_data->access_point = access_point;
        proc_data->proc_time_info = proc_info->time;
    }
    else 
    {
        proc_data = (hash_data*) malloc(sizeof(hash_data));
        proc_data->access_point = access_point;
        proc_data->proc_time_info = proc_info->time;

        new_entry.key = proc_info->pid;
        new_entry.data = proc_data;
        hash_insert(new_entry); 
    }
}

void initialize_task_counters(task_counter *tasks)
{
    tasks->valid_counter    = 0;
    tasks->running_counter  = 0;
    tasks->sleeping_counter = 0;
    tasks->stopped_counter  = 0;
    tasks->zombie_counter   = 0;
}

process_info** proc_table_generator(task_counter *tasks, unsigned short access_point)
{
    DIR *directory;
    struct dirent *directory_entry;
    int proc_dir_length = strlen(PROC_DIR);
    int stat_dir_length = strlen(STAT_DIR);
    char* stat_file_path;
    process_info **process_info_table = (process_info **) malloc(sizeof(process_info*));
    process_info *proc_info;
    static long long unsigned cpu_previous_time = 0;
    long long unsigned cpu_time = 0;
    long long unsigned cpu_total = cpu_total_time();
    cpu_time = cpu_total - cpu_previous_time;
    cpu_previous_time = cpu_total;
    
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
                    read_proc_username(proc_info, stat_file_path);
                    process_info_table = (process_info **) realloc(process_info_table, ++(tasks->valid_counter) * sizeof(process_info*));
                    process_info_table[tasks->valid_counter - 1] = proc_info;
                    // Stores the previous time info of the process to calculate the cpu usage per proc
                    time_table_update(proc_info, access_point, cpu_time);
                    update_state_counters(proc_info->state, tasks);
                }
                free(stat_file_path); 
            }
        }
        process_info_table = (process_info **) realloc(process_info_table, ++(tasks->valid_counter) * sizeof(process_info*));
        // marks the end of the list
        process_info_table[tasks->valid_counter - 1] = NULL;

        closedir(directory);
        return process_info_table;
    } 
    else 
    {
        snprintf(error_buffer, BUFFER_SIZE, "Error in process_manager => opendir(\"./proc\")");
		perror(error_buffer); 
        free(process_info_table);
        return NULL;     
    }
}

void update_state_counters(char state, task_counter* tasks)
{
    if(state == 'R')
        tasks->running_counter++;
    else if(state == 'S')
        tasks->sleeping_counter++;
    else if(state == 'T' || state == 't')
        tasks->stopped_counter++;
    else if(state == 'Z')
        tasks->zombie_counter++;
}

char* get_stat_file_path(char* pid, int proc_dir_length, int stat_dir_length)
{
    int directory_entry_length = strlen(pid);
    int proc_path_length = proc_dir_length + stat_dir_length + directory_entry_length + 4; // for "/" and '\0' and "us" of status
    char *stat_file_path = (char *) calloc(proc_path_length, sizeof(char));
    sprintf(stat_file_path, "%s/%s%s", PROC_DIR, pid, STAT_DIR);

    return stat_file_path;
}

void parse_command(char **string) 
{
  char* c = *string;
  int str_len  = strlen(*string) - 1;
  c += str_len;
  *string =  realloc(*string, str_len);
  // consome o ')'
  *c = '\0';
}

process_info* read_proc_stat(const char * stat_file)
{
    FILE * stat_filestream;
    int handle_file_return = handle_file_open(&stat_filestream, "r", stat_file);

    if(handle_file_return != 0) 
        return NULL;
        
    process_info* process = (process_info *) malloc(sizeof(process_info));
    process->command = (char*) malloc(COMMAND_MAX);  
    fscanf(stat_filestream, "%d%s%*c%c", &(process->pid), process->command, &process->state);
    // parse_command(&(process->command));
    // printf("command :%s\n", process->command);
    //This Process is dead, move on
    if(process->state == 'X')
    {
        fclose(stat_filestream);
        free(process);
        return NULL;
    }   
    else
    {
        for(int i = 0; i < TRASH_SIZE; i++)  fscanf(stat_filestream ,"%*u");
        fscanf(stat_filestream ,"%lu%lu", &(process->time.user_time), &(process->time.kernel_time));
        fscanf(stat_filestream, "%*d%*d");
        fscanf(stat_filestream, "%d%d"  , &(process->priority), &(process->nice));
        fscanf(stat_filestream, "%*d%*d");
    }
    fclose(stat_filestream);
    return process;
}

void read_proc_username(process_info* proc_info, char *stat_file) 
{   
    uid_t user_id = read_proc_uid(stat_file);
    proc_info->user_name = (char*) malloc(USER_NAME_MAX);
    struct passwd *result;
    struct passwd password_struct; 
    long buffer_size = sysconf(_SC_GETPW_R_SIZE_MAX);
    
    if (buffer_size == -1)          /* Value was indeterminate */
        buffer_size = 16384;        /* Should be more than enough */

    char *password_buffer= malloc(buffer_size);

    int getpwuid_error = getpwuid_r(user_id, &password_struct, password_buffer, buffer_size, &result);

    if (result == NULL) 
    {
        if (getpwuid_error == 0)
            strcpy(proc_info->user_name, "UNKNOWN");
        else {
            errno = getpwuid_error;
            // perror("getpwnam_r");
        }
    }
    else
    {
        if(strlen(password_struct.pw_name) > 9) 
        {
            memcpy(proc_info->user_name, password_struct.pw_name, 9);
            proc_info->user_name[8] = '+';
            proc_info->user_name[9] = '\0';
        }
        else 
            strcpy(proc_info->user_name, password_struct.pw_name);
    }   

    free(password_buffer);
}

uid_t read_proc_uid(char * stat_file)
{   
    // complete the stat to "us"
    uid_t user_id = 0;
    strcat(stat_file, "us");
    FILE* filestream; 
    char line_buffer[32];   
    int file_open_return = handle_file_open(&filestream, "r", stat_file);

    if(file_open_return == -1) 
    {
        return 0;
    }
    
    fscanf(filestream, "%s", line_buffer);
    // consume the file while we didn't find the Uid
    while(strcmp(line_buffer, "Uid:") != 0)
    {
        fscanf(filestream, "%s", line_buffer);
    }
    fscanf(filestream, "%*s%*u%u", &user_id);
    fclose(filestream);
    return user_id;
}

int handle_file_open(FILE **file_stream, const char* mode, const char *file_name) 
{	
	if(file_stream != NULL) 
	{
		*file_stream = fopen(file_name, mode);
		if (*file_stream == NULL) 
		{
			snprintf(error_buffer, BUFFER_SIZE, "Could not open file \"%s\"", file_name);
			// perror(error_buffer);
			return -1;
		}
	}	
	return 0;
}

void clear_time_table(hash_entry* time_table)
{
    hash_data* data;
    for (int i = 0; i < HASH_TABLE_MAX; i++)
    {   
        if(time_table[i].key != -1)
        {
            data = (hash_data*) time_table[i].data;
            free(data);
        }
    }
    free(time_table);
}

void clear_process_info_table(process_info ** process_info_table, int table_size)
{
    for(int i = table_size - 2; i >= 0; i--)
    {   
        free(process_info_table[i]->user_name);
        free(process_info_table[i]->command);
        free(process_info_table[i]); 
    }
    free(process_info_table);
}

long long unsigned cpu_total_time()
{
    FILE* proc_stat_filestream;
    long long unsigned user, nice, system, idle, total_time;

    int file_open_return = handle_file_open(&proc_stat_filestream, "r", PROC_STAT_PATH);
    
    if(file_open_return == -1)
    {   
        snprintf(error_buffer, BUFFER_SIZE, "Could not calculate CPU total time => cpu_total_time()");
		// perror(error_buffer);
        return -1;
    }

    fscanf(proc_stat_filestream,"%*s %llu %llu %llu %llu", &user, &nice, &system, &idle);
    total_time = user + system + system + idle; 

    fclose(proc_stat_filestream);

    return total_time;
}

float get_cpu_usage(time_info proc_time_previous, time_info proc_time_current, long long unsigned cpu_time)
{   
    long long unsigned total_time_current = proc_time_current.user_time + proc_time_current.kernel_time;
    long long unsigned total_time_previous = proc_time_previous.user_time + proc_time_previous.kernel_time; 
    double cpu_usage = 100;
    if(cpu_time != 0)
        cpu_usage = 100.0 * (float) ((double) (total_time_current - total_time_previous)  /(double) cpu_time);

    return cpu_usage * num_cpus;
}

unsigned int get_num_cpus()
{
    FILE* cpu_info_file;
    char parse_buffer[32];
    unsigned int num_cpus;
    int file_open_return = handle_file_open(&cpu_info_file, "r", PROC_CPUINFO_FILE);
    
    if(file_open_return == -1)
        return 1;
        
    do
    {
      fscanf(cpu_info_file, "%s", parse_buffer);
    } while(strcmp(parse_buffer, "siblings"));

    fscanf(cpu_info_file, "\t: %u", &num_cpus);

    fclose(cpu_info_file);

    return num_cpus;
}

static int compare_processes(const void * a, const void * b) 
{   
    process_info* proc_a = *((process_info **) a);
    process_info* proc_b = *((process_info **) b);
    int x = proc_a->cpu_percentage * 100;
    int y = proc_b->cpu_percentage * 100;
    int diff = (y - x);
    
    // sort by pid
    if(diff == 0) 
    {
        int pid_a = proc_a->pid;
        int pid_b = proc_b->pid;
        return pid_a - pid_b;
    }
    else
        return diff;
}

void write_info_in_memory(task_counter tasks, process_info** process_info_table, char* shared_memory)
{ 
    // write tasks
    unsigned print_offset = 0;
    char* memory_cursor = shared_memory;
    sprintf(memory_cursor, "%u %u %u %u %u\n%n", tasks.running_counter, tasks.sleeping_counter, 
        tasks.stopped_counter, tasks.valid_counter, tasks.zombie_counter, &print_offset);
    // loop through process_info_table
    memory_cursor += print_offset;
    for(int i = 0; i < tasks.valid_counter - 1; i++) 
    {
        // write every row
        process_info* proc_info = process_info_table[i];
        // parse_command(&(proc_info->command));
        sprintf(memory_cursor, "%d %s %d %d %c %3.2f %u %s\n%n", proc_info->pid, proc_info->user_name,proc_info->priority, 
            proc_info->nice, proc_info->state, proc_info->cpu_percentage, (unsigned) (proc_info->cpu_time * 100/sysconf(_SC_CLK_TCK)), 
            proc_info->command, &print_offset);
        memory_cursor += print_offset;
    }
}

void print_process_info_table(process_info **proc_info_table, task_counter tasks, short write_to_file) 
{   
    FILE* write_file;
    int file_open_result = handle_file_open(&write_file, "w", OUTPUT_FILE_PATH);

    if(file_open_result == -1)
        return;

    FILE* output = write_to_file == 0 ? stdout : write_file; 
    int size = tasks.valid_counter;

    fprintf(output, "%u %u %u %u %u ", tasks.running_counter, tasks.sleeping_counter, 
        tasks.stopped_counter, tasks.valid_counter, tasks.zombie_counter);

    for(int i = 0; i < size - 1; i++)
        fprintf(output, "%d %s %d %d %c %3.2f %u %s\n", proc_info_table[i]->pid, 
            proc_info_table[i]->user_name, proc_info_table[i]->priority, proc_info_table[i]->nice,
            proc_info_table[i]->state,proc_info_table[i]->cpu_percentage,
            (unsigned) (proc_info_table[i]->cpu_time*100/sysconf(_SC_CLK_TCK)), proc_info_table[i]->command);      
        
    fclose(write_file);
}
