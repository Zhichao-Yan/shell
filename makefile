CC = gcc
CFLAGS = -g -Wall


all: shell test

shell: shell.c
	$(CC) $(CFLAGS) -o shell shell.c

test: test.c
	$(CC) $(CFLAGS) -o test test.c

clean:
	rm -f shell test