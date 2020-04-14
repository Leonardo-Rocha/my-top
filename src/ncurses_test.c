#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#define HEADER_COLOR 1

void print_samples( int start_index, WINDOW* list_window);

int main()
{
    int max_rows, max_cols, current_col, current_row;
    WINDOW *summary_window;
    WINDOW *list_window; 
    int c;
    int start_index = 0;
    int quit = 0;
    
    initscr();             // Start curses mode 
    noecho();
    halfdelay(20);			  
    keypad(stdscr, TRUE); // enables keypad to use the arrow keys to scroll on the process list 
	curs_set(0);
    start_color();
    init_pair(HEADER_COLOR, COLOR_BLACK, COLOR_WHITE);

	getmaxyx(stdscr, max_rows, max_cols);

    summary_window = newwin(1, max_cols, 0, 0);
	list_window = newwin(max_rows - 3, max_cols, 4, 0);

	attron(COLOR_PAIR(HEADER_COLOR));
	mvprintw(2, 0, " PID\tUSER     \t PR\t NI\tS\t  %%CPU\t    TIME+\tCOMMAND");
    getyx(stdscr, current_row, current_col);
	current_row++;
	for(int i = current_col; i < max_cols; i++) addch(' ');
	refresh();
	attroff(COLOR_PAIR(HEADER_COLOR));
    
    while(1){
        print_samples(start_index, list_window);
        c = getch();
        switch (c)
        {
        case ERR:
            mvprintw(3, 0, " NO SIG READ");
            break;
        case KEY_DOWN:
            mvprintw(3,0, "KEY DOWN READ");
            if(start_index < 10)
                start_index++;
            break;
        case KEY_UP:
            mvprintw(3,0, "KEY UP READ");
            if(start_index > 0)
                start_index--;
            break;       
        case 'b':
            quit = 1;
            break;
        default:
            mvprintw(3,0, " TRIED TO READ %c",c);
            break;
        }
        refresh();
        if(quit)
            break;
    }
    
    delwin(summary_window);
	delwin(list_window);

	endwin(); // End curses mode

}

void print_samples( int start_index, WINDOW* list_window)
{	
	for(int i = start_index; i < start_index + 10; i++)
	{	
		mvwprintw(list_window, 4+ i - start_index, 0, "%d\n", i );
	}

	wrefresh(list_window);
}
