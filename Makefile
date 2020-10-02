CC=gcc
flags=-g -O0 -lm
out=main
in=main.c

all:
	$(CC) -o $(out) $(in) $(flags)

run:
	./$(out)

debug:
	gdb ./$(out)