#include <stdio.h>
#include <string.h>

#define PROC_CPUINFO_FILE "/proc/cpuinfo"
#define BUFFER_SIZE 200

char error_buffer[BUFFER_SIZE];

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

    printf("num_cpus = %u", num_cpus);
        
    return num_cpus;
}

int main()
{
    get_num_cpus();
    return 0;
}