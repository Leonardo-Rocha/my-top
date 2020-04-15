# MyTop

MyTop is a ncurses based implementation of the linux top.

## How to run

For the first use run make to compile and run the program. For further uses run ./build/my_top.

## Commands

### Arrow Up/Down

Use arrow up/down ou the mouse scroll to scroll through the processes list. The list is first sorted by CPU usage(↓) and then by PID(↑). 

### Kill

Press 'k' to start kill command. This commands sends the signal SIGKILL to the given PID. By default the process with highest CPU usage is chosen, by typing the user can enter another PID.

### Quit

Press 'q' or CTRL+C to quit.
