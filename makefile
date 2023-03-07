CC=g++
CFLAGS=-O2 -Wall -ansi -pedantic -std=c++11 -stdlib=libc++ 

bin/bootstrap: bin/bootstrap.o
	$(CC) -o bin/bootstrap bin/bootstrap.o

bin/bootstrap.o: src/bootstrap.cpp
	$(CC) -c -o bin/bootstrap.o src/bootstrap.cpp $(CFLAGS)

clean:
	@rm bin/bootstrap.o bin/bootstrap