CC = gcc
BU = ./build
RC = ./src

# Compiler flags
#
#-O2:			Turn on all optional optimizations except for loop
#			unrolling and function inlining.
#
#
#-Wall:			All of the `-W' options combined (all warnings on)
CCOPTS = -Wall -g
NCURSES = -lncurses

# Makefile targets

all: dir my_top text_interface process_manager run_top

dir: 
	mkdir -p $(BU)

my_top: $(RC)/my_top.c
	$(CC) $(CCOPTS) -o $(BU)/$@ $<

text_interface: $(RC)/text_interface.c 
	$(CC) $(CCOPTS) -o $(BU)/$@ $< $(NCURSES)

process_manager: $(RC)/process_manager.c $(RC)/hashtable.c
	$(CC) $(CCOPTS) -o $(BU)/$@ $^

# Run my_top
run_top: $(BU)/my_top
	$(BU)/my_top

# Clean up!
clean:
	rm -f $(BU)/*
