BIN_DIR= out
CC = g++
CFLAGS = -std=c++11 $(shell pkg-config --cflags opencv4) -fopenmp
LIBS = $(shell pkg-config --libs opencv4) -fopenmp

all: clean main
run: all
	./main 4 5
clean:
	rm -f main
	rm -f out/*
$(BIN_DIR)/main.o:
	${CC} $(CFLAGS) -c test.cpp -o $(BIN_DIR)/main.o

main: $(BIN_DIR)/main.o
	${CC} -o main $(BIN_DIR)/main.o $(LIBS)
