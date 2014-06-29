CFLAGS=-std=c99 -O3 -Wall
#INCLUDE=-I src/glew/include
#LIBRARY=-L src/glew/src
EXE=cubious
CC=gcc

all: main

run: all
	./$(EXE)

clean:
	rm *.o $(EXE)

main: main.o util.o noise.o map.o db.o sqlite3.o client.o
	$(CC) $(CFLAGS) main.o util.o noise.o map.o db.o client.o sqlite3.o -o $(EXE) $(LIBRARY) -lglfw3 -lpng -lGLEW -lGL -lGLU -lm -lpthread -ldl -lX11 -lXxf86vm -lXrandr -lXi

main.o: 
	$(CC) $(CFLAGS) $(INCLUDE) -c -o main.o src/main.c
	
util.o:
	$(CC) $(CFLAGS) $(INCLUDE) -c -o util.o src/util.c
	
noise.o: 
	$(CC) $(CFLAGS) $(INCLUDE) -c -o noise.o src/noise.c
	
map.o: 
	$(CC) $(CFLAGS) $(INCLUDE) -c -o map.o src/map.c
	
db.o: 
	$(CC) $(CFLAGS) $(INCLUDE) -c -o db.o src/db.c
	
client.o:
	$(CC) $(CFLAGS) $(INCLUDE) -c -o client.o src/client.c

sqlite3.o: 
	$(CC) $(CFLAGS) -c -o sqlite3.o src/sqlite3/sqlite3.c

