CC=g++
flags=-g -O0 -lm
out=main
in=main.cpp midi-parser.cpp

all:
	$(CC) -o $(out) $(in) $(flags)

run:
	./$(out)

debug:
	gdb ./$(out)