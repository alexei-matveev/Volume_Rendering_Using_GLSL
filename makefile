CC = g++
OFLAG = -o
CFLAGS = -c -Wall -g -DDEBUG
LIB = -lGLEW -lglut -lGL -lGLU


all: main.o
	$(CC) $(OFLAG) main main.o $(LIB)
main.o: main.cpp
	$(CC) $(CFLAGS) main.cpp
clean:
	rm main.o main
