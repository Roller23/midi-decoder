CC=g++
flags=-O3 -lm -std=c++17
out=main
in=main.cpp midi-parser.cpp

all:
	$(CC) -o $(out) $(in) $(flags)

run:
	./$(out)

debug:
	gdb ./$(out)