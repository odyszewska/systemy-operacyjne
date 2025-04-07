CC = gcc
CFLAGS = -Wall -std=c17 -g

all: zad1 zad2 test_zad1 test_zad2

zad1:
	$(CC) $(CFLAGS) -o zad1 zad1.c

zad2:
	$(CC) $(CFLAGS) -o zad2 zad2.c

test_zad1:
	./zad1 3
	./zad1 5
	./zad1 7

test_zad2:
	./zad2 .

clean:
	rm zad1 zad2

.PHONY: clean all