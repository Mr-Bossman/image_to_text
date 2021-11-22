BIN_DIR= out
CC = g++
CFLAGS = -std=c++11 $(shell pkg-config --cflags opencv4) -fopenmp -O3
LIBS = $(shell pkg-config --libs opencv4) -fopenmp

all: clean main
run: all
	./main 4 5
clean:
	rm -f main
	rm -f out/*
$(BIN_DIR)/main.o:
	${CC} $(CFLAGS) -c render.cpp -o $(BIN_DIR)/main.o

main: $(BIN_DIR)/main.o
	${CC} -o main $(BIN_DIR)/main.o $(LIBS)

$(BIN_DIR)/display.o:
	${CC} $(CFLAGS) -c display.cpp -o $(BIN_DIR)/display.o

display: $(BIN_DIR)/display.o
	${CC} -o display $(BIN_DIR)/display.o $(LIBS)
