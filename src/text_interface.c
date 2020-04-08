#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define SHARED_MEM_SIZE      63489
#define SHARED_MEM_KEY       56373
#define SHARED_MEM_FLAGS     S_IRWXG | S_IRWXU | S_ISUID | IPC_CREAT

#define HEADER_COLOR 1

typedef struct _WIN_struct {
	int startx, starty;
	int height, width;
} WIN;

void init_win_params(WIN *p_win);
void print_win_params(WIN *p_win);
void create_box(WIN *win, bool flag);

//TODO: ir sumindo com as colunas da direita conforme a janela for sendo reduzida
int main(int argc, char *argv[])
{	
	char *shared = (char*) atoll(argv[1]);
	
    //WINDOW *process_list_window; 
    WIN win;
    //int screen_rows, screen_columns;
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

    //PRIMEIRO: escrever informações número de tasks, quantas em running, sleeping, stopped e etc com o numero em A_BOLD

    //SEGUNDO: escrever o header PID USER PR NI VIRT RES SHR S %CPU %MEM TIME+ COMMAND
	attron(COLOR_PAIR(HEADER_COLOR));
	printw("PID USER PR NI S %%CPU TIME+ COMMAND");
	//TODO: preencher header com vazio até o fim da linha...
	refresh();
	attroff(COLOR_PAIR(HEADER_COLOR));
	
	//TERCEIRO: POPULAR LISTA INICIALMENTE COM OS 15 PRIMEIROS PIDs com maior uso de CPU
	
    //QUARTO: VERIFICAÇÃO DE BUFFER PARA COMANDOS COMO KILL, SCROLL UP E SCROLL DOWN
	while((ch = getch()) != 'q')
	{	
        switch(ch)
		{	
            //TODO: popular a lista com as linhas em ordem crescente de PID
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
				printw("Entrou no kill : %c", shared[1]);
				refresh();
				break;
			default:
				break;
		}
	}

	//Seta a flag de kill
	
	endwin(); // End curses mode
	
	// tells the other programs that text_interface has ended
	memcpy(shared, "\x01", sizeof(char));

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
