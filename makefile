CC=gcc

.PHONY: all

all: 2015110_shell

2015110_shell: 2015110_shell.c
	$(CC) 2015110_shell.c -o 2015110_shell
	gnome-terminal -e ./2015110_shell