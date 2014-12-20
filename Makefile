CFLAGS=-std=c99 -O3
#INCLUDE=-I src/glew/include
#LIBRARY=-L src/glew/src
EXE=cubious
SERVEXE=server
SERVFLAGS=-lm -lpthread -ldl
FLAGS=-lglfw -lpng -lGLEW -lGL -lGLU -lm -lpthread -ldl -lX11 -lXxf86vm -lXrandr -lXi
CC=gcc

all: main

run: all
	./$(EXE)

clean:
	rm *.o $(EXE) $(SERVEXE)

server: sqlite3.o server.o
	$(CC) $(CFLAGS) server.o sqlite3.o -o $(SERVEXE)  $(SERVFLAGS)
	
main: client sqlite3.o
	$(CC) $(CFLAGS) main.o util.o noise.o map.o db.o client.o sqlite3.o -o $(EXE) $(LIBRARY) $(FLAGS)

client: 
	$(CC) $(CFLAGS) $(INCLUDE) -c -o main.o src/main.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o util.o src/util.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o noise.o src/noise.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o map.o src/map.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o db.o src/db.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o client.o src/client.c

server.o:
	$(CC) -c -o server.o src/server.c

sqlite3.o: 
	$(CC) $(CFLAGS) -c -o sqlite3.o src/sqlite3/sqlite3.c

