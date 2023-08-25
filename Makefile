# Make the readico and readbmp programs
#
CC=gcc -o
DEBUG=-ggdb -DDEBUG
RM=rm -f

all : release debug

release: readico readbmp

debug: ri-db rb-db

ri-db: ico.c
	$(CC) ri-db $(DEBUG) ico.c

rb-db: bmp.c
	$(CC) rb-db $(DEBUG) bmp.c

readico: ico.c
	$(CC) readico ico.c

readbmp: bmp.c
	$(CC) readbmp bmp.c

.PHONY : clean
clean:
	$(RM) readico readbmp ri-db rb-db
