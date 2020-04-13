#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define COMMAND_MAX      64
#define USER_NAME_MAX    10
#define HEADER_COLOR 1

typedef struct _WIN_struct {
	int startx, starty;
	int height, width;
} WIN;

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

void init_win_params(WIN *p_win);
void print_win_params(WIN *p_win);
void create_box(WIN *win, bool flag);
void read_summary_from_memory(task_counter* tasks, char* shared_memory);
void print_summary(task_counter tasks, WINDOW* summary_window);

void print_top_table(process_info** proc_info_table, int start_index, WINDOW* process_list_window);

// formats seconds to minutes:seconds 
char* format_time(unsigned int time);

//TODO: ir sumindo com as colunas da direita conforme a janela for sendo reduzida
int main(int argc, char *argv[])
{	
	int memory_segment_id;
	int previous_num_proc = 0, start_index = 0;
	char* shared_memory;
	process_info** process_info_table;
	task_counter tasks; 

	int row, col;

    if(argc != 2) {
        printf("Usage: ./text_interface <segment_id>");
        exit(-1);
    }

	memory_segment_id = atoi(argv[1]);
    //printf("\nSegment ID in process manager : %d\n", segment_id);

	/** attach the shared memory segment */
	shared_memory = (char *) shmat(memory_segment_id, NULL, 0);
	//printf("shared memory segment %d attached at address %p\n", segment_id, shared_memory);
	
	WINDOW *summary_window;
	//TODO: set process list window position after header
    WINDOW *process_list_window; 
    WIN win;

    int ch; 

	initscr();			  // Start curses mode 
    cbreak();             // Line buffering disabled 
    noecho();             // Don't echo while we do getch() 
    keypad(stdscr, TRUE); // enables keypad to use the arrow keys to scroll on the process list 
    start_color();
    init_pair(HEADER_COLOR, COLOR_BLACK, COLOR_WHITE);

    //Initialize the window parameters 
	init_win_params(&win);
	print_win_params(&win);
	getmaxyx(stdscr, row, col);
	attron(COLOR_PAIR(HEADER_COLOR));
	mvprintw(2, 0, "PID USER PR NI S %%CPU TIME+ COMMAND");
	// fills the rest of the header with HEADER_COLOR
	for(int i = 0; i < col; i++) addch(' ');
	refresh();
	attroff(COLOR_PAIR(HEADER_COLOR));
	
	// Wait process_manager write info in memory
	sleep(1);
	read_summary_from_memory(&tasks, shared_memory);
	process_info_table = allocate_proc_list(tasks.valid_counter);
	previous_num_proc = tasks.valid_counter;
    //TERCEIRO: VERIFICAÇÃO DE BUFFER PARA COMANDOS COMO KILL, SCROLL UP E SCROLL DOWN
	while((ch = getch()) != 'q')
	{	
		sleep(1);
		read_summary_from_memory(&tasks, shared_memory);
		process_info_table = reallocate_proc_list(process_info_table, tasks.valid_counter, previous_num_proc);
		previous_num_proc = tasks.valid_counter;
		read_process_list_from_memory(process_info_table, shared_memory, tasks.valid_counter);
		print_summary(tasks, summary_window);
		print_top_table(process_info_table, start_index, process_list_window);
        switch(ch)
		{	
			case KEY_UP:
				--win.starty;
				break;
			case KEY_DOWN:
				++win.starty;
				break;
			case KEY_LEFT:
				--win.startx;
				break;
			case KEY_RIGHT:
				++win.startx;
				break;
			case 'k':
				printw("Entrou no kill!");
				refresh();
				break;
			default:
				break;
		}
	}
	
	endwin(); // End curses mode
	
	/** now detach the shared memory segment */ 
	if ( shmdt(shared_memory) == -1) 
    {
		fprintf(stderr, "Unable to detach\n");
	}

	return 0;
}

void init_win_params(WIN *p_win)
{
	p_win->height = 3;
	p_win->width = 10;
	p_win->starty = (LINES - p_win->height)/2;	
	p_win->startx = (COLS - p_win->width)/2;
}

void print_win_params(WIN *p_win)
{
#ifdef _DEBUG
	mvprintw(25, 0, "%d %d %d %d ", p_win->startx, p_win->starty, 
				p_win->width, p_win->height);
	refresh();
#endif
}

void create_line(WIN *p_win)
{	
	//int i, j;
	//int x, y, w, h;

	//x = p_win->startx;
	//y = p_win->starty;
	//w = p_win->width;
	//h = p_win->height;
				
	refresh();
}

process_info** allocate_proc_list(int num_proc)
{
	process_info ** proc_info_table = (process_info**) malloc(num_proc*sizeof(process_info*));
	for(int i = 0; i < num_proc - 1; i++) 
        proc_info_table[i] = (process_info*) malloc(sizeof(process_info));
	return proc_info_table;
}

process_info ** reallocate_proc_list(process_info ** proc_info_table, int num_proc, int previous_num_proc)
{
	if(num_proc > previous_num_proc)
	{
		proc_info_table = (process_info **) realloc(proc_info_table, num_proc * sizeof(process_info*));	
		for(int i = previous_num_proc; i < num_proc - 1; i++) 
        	proc_info_table[i] = (process_info*) malloc(sizeof(process_info));
	}
	else if(previous_num_proc > num_proc)
	{
		for(int i = previous_num_proc-1; i >= num_proc; i--) 
        	free(proc_info_table[i]);
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
    sscanf(shared_memory, "%u%u%u%u%u", &(tasks->running_counter), &(tasks->sleeping_counter), 
        &(tasks->stopped_counter), &(tasks->valid_counter), &(tasks->zombie_counter));
}

void read_process_list_from_memory(process_info** process_info_table, char* shared_memory, int num_proc)
{
	// loop through memory writing to process_info_table
	for(int i = 0; i < num_proc - 1; i++) 
    {
		// write every row
        process_info* proc_info = process_info_table[i];
		//TODO: REMOVE \t's
        sscanf(shared_memory, "%d\t%s\t%d\t%d\t%c\t%3.2f\t%.2f\t%s\n", &(proc_info->pid), proc_info->user_name,
			&(proc_info->priority), &(proc_info->nice), &(proc_info->state), &(proc_info->cpu_percentage), 
			&(proc_info->cpu_time), proc_info->command);
    }
}

void print_summary(task_counter tasks, WINDOW* summary_window)
{	
	wclear(summary_window);
	mvwprintw(summary_window, 0, 0, "Tasks: ");
	attron(A_BOLD);
	wprintw(summary_window, "%d", tasks.valid_counter);
	attroff(A_BOLD);
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

void print_top_table(process_info** proc_info_table, int start_index, WINDOW* process_list_window)
{	
	//TODO: calculate this based on window size
	int max_process = 15;
	int row = 0;
	process_info* proc_info;
	for(int i = start_index; i < max_process; i++)
	{	
		//TODO: do right calculus
		row = i;
		proc_info = proc_info_table[i];
		//TODO: use correct align
		mvwprintw(process_list_window, row, 0, "%d\t%s\t%d\t%d\t%c\t%3.2f\t%s\t%s\n", proc_info->pid, 
			proc_info->user_name, proc_info->priority, proc_info->nice, proc_info->state, 
			proc_info->cpu_percentage, format_time(proc_info->cpu_time), proc_info->command);
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
		sprintf(formatted_time, "%3d:0%d:%02d", time, seconds, centi_secs);
	else
		sprintf(formatted_time, "%3d:%d:%02d", time, seconds, centi_secs);
	
	return formatted_time;
}
