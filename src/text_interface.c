#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <signal.h>

#define INPUT_FILE_PATH  "output.txt"
#define COMMAND_MAX      64
#define USER_NAME_MAX    10
#define HEADER_COLOR     1
#define BUFFER_SIZE      200
#define MAX_ITERS 		 1000

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
    unsigned int pid;
    char user_name[USER_NAME_MAX]; 
    char state;
    int priority;
    int nice;
    float cpu_percentage; 
    unsigned cpu_time;
    char command[COMMAND_MAX]; 
} process_info;

char error_buffer[BUFFER_SIZE];

//void init_win_params(WIN *p_win);
//void print_win_params(WIN *p_win);

void read_summary_from_memory(task_counter* tasks, char* shared_memory);

void read_process_list_from_memory(process_info** process_info_table, char* shared_memory, int num_proc);

void read_summary_from_file(FILE* read_file, task_counter* tasks);

void read_processes_from_file(FILE* read_file, int num_proc, process_info** process_info_table);

void print_summary(task_counter tasks, WINDOW* summary_window);

void print_top_table(process_info** proc_info_table, int num_proc, int start_index, WINDOW* process_list_window);

/* formats seconds to minutes:seconds.cents */
char* format_time(unsigned int time);

process_info** allocate_proc_list(int num_proc);

process_info ** reallocate_proc_list(process_info ** proc_info_table, int num_proc, int previous_num_proc);

void clear_process_info_table(process_info ** process_info_table, int table_size);

/* Calls fopen and returns zero if the file was opened succesfully and -1 on error, also printing to stderr. */
int handle_file_open(FILE **file_stream, const char* mode, const char *file_name);

/* Test allocs and reallocs of process_list_info */
void test_allocations();

/*								*/
void test_sucessive_reads();

int main(int argc, char *argv[])
{	
	int memory_segment_id;
	char* shared_memory;

	process_info** process_info_table;
	task_counter tasks; 

	int max_rows, max_cols, current_col, current_row, quit = 0;
	int previous_num_proc = 0, processes_start_y = 0;
	int ch;
	long strtol_result;
	pid_t pid_to_kill;
	char read_string[32] = "\n";
	// FILE* read_file;

    if(argc != 2) 
	{
        printf("Usage: ./text_interface <segment_id>");
        exit(-1);
    }

	memory_segment_id = atoi(argv[1]);
    //printf("\nSegment ID in process manager : %d\n", segment_id);

	/** attach the shared memory segment */
	shared_memory = (char *) shmat(memory_segment_id, NULL, 0);
	//printf("shared memory segment %d attached at address %p\n", segment_id, shared_memory);
	
	WINDOW *summary_window;
    WINDOW *process_list_window; 

	initscr();			  // Start curses mode 
    raw();                // Line buffering disabled
	halfdelay(20);		  // When we do getch(), wait for 2 seconds before return ERR value		 
    noecho();             // Don't echo while we do getch() 
    keypad(stdscr, TRUE); // enables keypad to use the arrow keys to scroll on the process list 
	curs_set(0);
    start_color();
    init_pair(HEADER_COLOR, COLOR_BLACK, COLOR_WHITE);

	getmaxyx(stdscr, max_rows, max_cols);

	// initialize the windows
	summary_window = newwin(1, max_cols, 0, 0);
	process_list_window = newwin(max_rows - 3, max_cols, 3, 0);

	// prints header
	attron(COLOR_PAIR(HEADER_COLOR));
	mvprintw(2, 0, " PID\tUSER     \t PR\t NI\tS\t%%CPU  \t    TIME+\tCOMMAND");
	// fills the rest of the header with HEADER_COLOR
	getyx(stdscr, current_row, current_col);
	current_row++;
	for(int i = current_col; i < max_cols; i++) addch(' ');
	refresh();
	attroff(COLOR_PAIR(HEADER_COLOR));

	//Wait process_manager write info in memory
	sleep(2);
	
	read_summary_from_memory(&tasks, shared_memory); 
	// handle_file_open(&read_file, "r", INPUT_FILE_PATH);
	// read_summary_from_file(read_file, &tasks);
	process_info_table = allocate_proc_list(tasks.valid_counter);
	previous_num_proc = tasks.valid_counter;
	while(1)
	{	
		read_summary_from_memory(&tasks, shared_memory);
		// read_summary_from_file(read_file, &tasks);
		process_info_table = reallocate_proc_list(process_info_table, tasks.valid_counter, previous_num_proc);
		previous_num_proc = tasks.valid_counter;

		read_process_list_from_memory(process_info_table, shared_memory, tasks.valid_counter);
		// read_processes_from_file(read_file, tasks.valid_counter, process_info_table);

		getmaxyx(process_list_window, max_rows, max_cols);
		print_summary(tasks, summary_window);
		print_top_table(process_info_table, tasks.valid_counter, processes_start_y, process_list_window);
		
		ch = getch();
		
		// TODO: corrigir linha não limpando. 
		move(1, 0);
		clrtoeol();
		refresh();
		// buffer verification for commands like kill, arrow keys and quit
		switch(ch)
		{	
			case KEY_UP:
				if(processes_start_y > 0)
					processes_start_y--;
				break;
			case KEY_DOWN:
				if(processes_start_y < (tasks.valid_counter - max_rows))
					processes_start_y++;
				break;
			case KEY_LEFT:
				//TODO: ir sumindo com as colunas da direita conforme 
				// decrease process_start_y
				break;
			case KEY_RIGHT:
				// SUMIR COM AS DA ESQUERDA
				// increase process_start_x
				break;
			case 'k':
				pid_to_kill = (pid_t) process_info_table[0]->pid;
				mvprintw(1, 0, "PID to kill [default pid = %d]: ", pid_to_kill);
				refresh();
				// turn the echo on so the user can see what he's typing
				echo();
				scanw("%s", read_string);
				strtol_result = strtol(read_string, NULL, 10);
				if(strtol_result)
				{	
					pid_to_kill = (pid_t) strtol_result;
					kill(pid_to_kill, SIGKILL);
					mvprintw(1, 0, "SIGKILL sent to PID %d]: ", pid_to_kill);
				}
				else if(strtol_result == 0 && read_string[0] == '\n') 
				{
					kill(pid_to_kill, SIGKILL);
					mvprintw(1, 0, "SIGKILL sent to PID %d]: ", pid_to_kill);
				}
				else 
				{
					mvprintw(1, 0, "Invalid integer!");
				}
				refresh();				
				noecho();		
				break;
			case 'q':
				quit = 1;
				break;
			case ERR:
				break;
			default:
				mvprintw(1, 0, "Unknown command - read README for more info");
				refresh();
				break;
		}
		if(quit)
			break;
	}
	clear_process_info_table(process_info_table, tasks.valid_counter);
	
	delwin(summary_window);
	delwin(process_list_window);

	endwin(); // End curses mode

	/** now detach the shared memory segment */ 
	if ( shmdt(shared_memory) == -1) 
    {
		fprintf(stderr, "Unable to detach\n");
	}

	// fclose(read_file);

	return 0;
}

void test_sucessive_reads()
{
	FILE* read_file;
	int num_proc = 0,previous_proc = 0;
	task_counter tasks; 

	handle_file_open(&read_file, "r", INPUT_FILE_PATH);
	
	// tests with normal size
	read_summary_from_file(read_file, &tasks);
	num_proc = tasks.valid_counter;
	process_info** proc_info_table = allocate_proc_list(num_proc);
	read_processes_from_file(read_file, num_proc, proc_info_table);
	previous_proc = num_proc;
	
	// test with smaller table size to realloc
	fseek(read_file, 0, SEEK_SET);
	read_summary_from_file(read_file, &tasks);
	num_proc = tasks.valid_counter - 100;
	proc_info_table = reallocate_proc_list(proc_info_table, num_proc, previous_proc);
	read_processes_from_file(read_file, num_proc, proc_info_table);
	previous_proc = num_proc;
	
	// test with larger table size to realloc -> bigger realloc
	fseek(read_file, 0, SEEK_SET);
	read_summary_from_file(read_file, &tasks);
	num_proc = previous_proc + 50;
	proc_info_table = reallocate_proc_list(proc_info_table, num_proc, previous_proc);
	read_processes_from_file(read_file, num_proc, proc_info_table);
	previous_proc = num_proc;
	
	fclose(read_file);
	clear_process_info_table(proc_info_table, num_proc);
}

void clear_process_info_table(process_info ** process_info_table, int table_size)
{
    for(int i = table_size - 1; i >= 0; i--)
    {   
        free(process_info_table[i]); 
		process_info_table[i] = NULL;
    }
    free(process_info_table);
}

process_info** allocate_proc_list(int num_proc)
{
	process_info ** proc_info_table = (process_info**) malloc(num_proc*sizeof(process_info*));
	for(int i = 0; i < num_proc; i++) 
        proc_info_table[i] = (process_info*) malloc(sizeof(process_info));
	return proc_info_table;
}

process_info ** reallocate_proc_list(process_info ** proc_info_table, int num_proc, int previous_num_proc)
{
	if(num_proc > previous_num_proc)
	{
		proc_info_table = (process_info **) realloc(proc_info_table, num_proc * sizeof(process_info*));	
		for(int i = previous_num_proc; i < num_proc; i++) 
        	proc_info_table[i] = (process_info*) malloc(sizeof(process_info));
	}

	else if(previous_num_proc > num_proc)
	{
		for(int i = previous_num_proc - 1; i >= num_proc; i--)
		{
			if(proc_info_table[i] != NULL)
			{
        		free(proc_info_table[i]);
				proc_info_table[i] = NULL;
			} 
		}
		proc_info_table = (process_info **) realloc(proc_info_table, num_proc * sizeof(process_info*));
	}
	
	return proc_info_table;
}

void test_allocations()
{
	int num_procs_test = 200;
	process_info** proc_info_table =  allocate_proc_list(num_procs_test);
	for(int i = 0; i < num_procs_test; i++)
		assert(proc_info_table[i] != NULL);
	proc_info_table = reallocate_proc_list(proc_info_table, num_procs_test + 50, num_procs_test);
	num_procs_test = num_procs_test + 50;
	for(int i = 0; i < num_procs_test; i++)
		assert(proc_info_table[i] != NULL);
	proc_info_table = reallocate_proc_list(proc_info_table, num_procs_test - 100, num_procs_test);
	num_procs_test = num_procs_test - 100;
	for(int i = 0; i < num_procs_test; i++)
		assert(proc_info_table[i] != NULL);
}	

void read_summary_from_memory(task_counter* tasks, char* shared_memory)
{ 
    // read tasks
    sscanf(shared_memory, "%u %u %u %u %u ", &(tasks->running_counter), &(tasks->sleeping_counter), 
        &(tasks->stopped_counter), &(tasks->valid_counter), &(tasks->zombie_counter));
}

void read_summary_from_file(FILE* read_file, task_counter* tasks)
{ 
    // read tasks
	fseek(read_file, 0, SEEK_SET);
    fscanf(read_file, "%u %u %u %u %u\n", &(tasks->running_counter), &(tasks->sleeping_counter), 
        &(tasks->stopped_counter), &(tasks->valid_counter), &(tasks->zombie_counter));
}

void read_processes_from_file(FILE* read_file, int num_proc, process_info** process_info_table)
{ 
	for(int i = 0; i < num_proc; i++) 
    {
		// write every row
        process_info* proc_info = process_info_table[i];
        fscanf(read_file, "%d %s %d %d %c %f %u %s\n", &(proc_info->pid), proc_info->user_name,
			&(proc_info->priority), &(proc_info->nice), &(proc_info->state), &(proc_info->cpu_percentage), 
			&(proc_info->cpu_time), proc_info->command);
    }
}

void read_process_list_from_memory(process_info** process_info_table, char* shared_memory, int num_proc)
{
	// loop through memory writing to process_info_table
	int offset = 0;
	char *current_string_address = shared_memory;

	// offsets from the summary
	sscanf(shared_memory, "%*u %*u %*u %*u %*u %n", &offset);
	current_string_address += offset;

	// loop through processes
	for(int i = 0; i < num_proc - 1; i++) 
    {
		// read every proc
        process_info* proc_info = process_info_table[i];
        sscanf(current_string_address, "%d %s %d %d %c %f %u %s\n%n", &(proc_info->pid), proc_info->user_name,
			&(proc_info->priority), &(proc_info->nice), &(proc_info->state), &(proc_info->cpu_percentage), 
			&(proc_info->cpu_time), proc_info->command, &offset);
		// updates string address
		current_string_address += offset;
    }
}

void print_summary(task_counter tasks, WINDOW* summary_window)
{	
	wclear(summary_window);
	mvwprintw(summary_window, 0, 0, "Tasks: ");
	attron(A_BOLD);
	wprintw(summary_window, "%d", tasks.valid_counter);
	attroff(A_BOLD);
	wrefresh(summary_window);
	wprintw(summary_window, " total,\t");
	
	attron(A_BOLD);
	wprintw(summary_window, "%d ", tasks.running_counter);
	attroff(A_BOLD);
	wprintw(summary_window, "running, ");

	attron(A_BOLD);
	wprintw(summary_window, "%d ", tasks.sleeping_counter);
	attroff(A_BOLD);
	wprintw(summary_window, "sleeping, ");

	attron(A_BOLD);
	wprintw(summary_window, "%d ", tasks.stopped_counter);
	attroff(A_BOLD);
	wprintw(summary_window, "stopped, ");

	attron(A_BOLD);
	wprintw(summary_window, "%d ", tasks.zombie_counter);
	attroff(A_BOLD);
	wprintw(summary_window, "zombie");
	
	wrefresh(summary_window);
}

void print_top_table(process_info** proc_info_table, int num_proc, int start_index, WINDOW* process_list_window)
{	
	int max_process = 0, max_columns = 0;
	max_columns++;
	getmaxyx(process_list_window, max_process, max_columns);
	process_info* proc_info;
	char * time_string;
	for(int i = start_index; i < (start_index + max_process - 1) && i < num_proc; i++)
	{	
		proc_info = proc_info_table[i];
		time_string = format_time(proc_info->cpu_time);
		mvwprintw(process_list_window, i - start_index, 0, "%4d\t%-9s\t%3d\t%3d\t%c\t%3.2f\t%9s\t%s\n", proc_info->pid, 
			proc_info->user_name, proc_info->priority, proc_info->nice, proc_info->state, 
			proc_info->cpu_percentage, time_string, proc_info->command);
		free(time_string);
	}

	wrefresh(process_list_window);
}
 
char* format_time(unsigned int time)
{
	char * formatted_time = calloc(16, sizeof(char));
	
	int centi_secs = time % 100;
	time = time / 100;
	int seconds = time % 60;
	time = time / 60;
	if(seconds < 10)
		sprintf(formatted_time, "%3d:0%d.%02d", time, seconds, centi_secs);
	else
		sprintf(formatted_time, "%3d:%d.%02d", time, seconds, centi_secs);
	
	return formatted_time;
}

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
