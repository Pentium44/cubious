FLAGS=-std=c99 -O3 -Wall
EXE=cubious

all: main

run: all
	./main

clean:
	rm *.o $(EXE)

main: sqlite3.o main.o util.o noise.o map.o db.o
	gcc $(FLAGS) main.o util.o noise.o map.o db.o sqlite3.o -o $(EXE) $(LIBRARY) -lglfw -lGLEW -lGL -lGLU -lm -lpthread -ldl

main.o: src/main.c
	gcc $(FLAGS) $(INCLUDE) -c -o main.o src/main.c
	
util.o: src/util.c
	gcc $(FLAGS) $(INCLUDE) -c -o util.o src/util.c
	
noise.o: src/noise.c
	gcc $(FLAGS) $(INCLUDE) -c -o noise.o src/noise.c
	
map.o: src/map.c
	gcc $(FLAGS) $(INCLUDE) -c -o map.o src/map.c
	
db.o: src/db.c
	gcc $(FLAGS) $(INCLUDE) -c -o db.o src/db.c
	
sqlite3.o: sqlite3/sqlite3.c
	gcc $(FLAGS) -c -o sqlite3.o sqlite3/sqlite3.c
