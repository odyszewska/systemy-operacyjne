CC = gcc
CFLAGS = -Wall -std=c17 -g
LIBS = -lpthread -lm

all: zad1 test_zad1

zad1:
	$(CC) $(CFLAGS) -o zad1 zad1.c $(LIBS)

test_zad1:
	./zad1 0.0000009 5
	./zad1 0.000000099 8
	./zad1 0.000000007 7
	./zad1 0.000000003 9
	./zad1 0.000000001 12
	./zad1 0.0000000001 6

clean:
	rm -f zad1

.PHONY: clean all test_zad1
