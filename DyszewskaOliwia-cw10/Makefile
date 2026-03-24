CC = gcc
CFLAGS = -Wall -std=c17 -g
LIBS = -lpthread -lm

all: main test

main:
	$(CC) $(CFLAGS) -o main main.c $(LIBS)

test:
	./main 20 3

clean:
	rm -f main

.PHONY: clean all
