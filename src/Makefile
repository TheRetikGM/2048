CC=gcc

all: 2048-curses.o dialog.o
	$(CC) 2048-curses.o dialog.o -lcurses -o 2048-curses

2048-curses.o: 2048-curses.c
	$(CC) 2048-curses.c -c

dialog.o: dialog.c
	$(CC) dialog.c -c

clean:
	rm -rf 2048-curses *.o
