CC=gcc
flags=-g -O0
out=main
in=main.c

all:
	$(CC) -o $(out) $(in) $(flags)

run:
	./$(out)