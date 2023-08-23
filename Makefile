# Make the readico program
#
release : ico.c
	gcc -o readico ico.c

debug: ico.c
	gcc -ggdb -o ri-db -DDEBUG ico.c

clean:
	rm readico ri-db
