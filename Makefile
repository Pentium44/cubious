FLAGS=-std=c99 -O3 -Wall
#INCLUDE=-I src/glew/include
#LIBRARY=-L src/glew/src
EXE=cubious

all: main

run: all
	./main

clean:
	rm *.o $(EXE)

main: main.o util.o noise.o map.o db.o sqlite3.o
	$(CC) $(FLAGS) main.o util.o noise.o map.o db.o sqlite3.o -o $(EXE) $(LIBRARY) -lglfw3 -lpng -lGLEW -lGL -lGLU -lm -lpthread -ldl -lX11 -lXxf86vm -lXrandr -lXi

main.o: 
	$(CC) $(FLAGS) $(INCLUDE) -c -o main.o src/main.c
	
util.o:
	$(CC) $(FLAGS) $(INCLUDE) -c -o util.o src/util.c
	
noise.o: 
	$(CC) $(FLAGS) $(INCLUDE) -c -o noise.o src/noise.c
	
map.o: 
	$(CC) $(FLAGS) $(INCLUDE) -c -o map.o src/map.c
	
db.o: 
	$(CC) $(FLAGS) $(INCLUDE) -c -o db.o src/db.c
	
sqlite3.o: 
	$(CC) $(FLAGS) -c -o sqlite3.o src/sqlite3/sqlite3.c

