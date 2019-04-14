CC = gcc
CFLAGS = -Wall -std=c11 -g -Iinclude -I../ -pedantic

all: list parser test

test: test.o list parser
	$(CC) $(CFLAGS) -o test test.o ./bin/liblist.so ./bin/libcal.so

test.o: test.c
	$(CC) $(CFLAGS) -c test.c

list: LinkedListAPI.o
	mkdir -p bin
	$(CC) -shared -o ./bin/liblist.so ./bin/LinkedListAPI.o

parser: CalendarParser.o LinkedListAPI.o
	mkdir -p bin
	$(CC) -shared -o ./bin/libcal.so ./bin/CalendarParser.o

LinkedListAPI.o: ./src/LinkedListAPI.c
	mkdir -p bin
	$(CC) $(CFLAGS) -c -fpic -o ./bin/LinkedListAPI.o ./src/LinkedListAPI.c

CalendarParser.o: list ./src/CalendarParser.c
	mkdir -p bin
	$(CC) $(CFLAGS) -c -fpic -o ./bin/CalendarParser.o ./src/CalendarParser.c

clean:
	rm -rf *.o *.so
	rm test
