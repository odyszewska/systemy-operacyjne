CC = gcc
CFLAGS = -Wall -std=c17

all: shared static client1 client2 client3

shared:
	$(CC) $(CFLAGS) -fPIC -c collatz.c
	$(CC) $(CFLAGS) -shared collatz.o -o libsharedlib.so

static:
	$(CC) $(CFLAGS) -c collatz.c
	ar cr staticlib.a collatz.o

client1:
	$(CC) $(CFLAGS) client.c staticlib.a -o client1

client2:
	$(CC) $(CFLAGS) client.c -L. -lsharedlib -Wl,-rpath,. -o client2

client3:
	$(CC) $(CFLAGS) -DDYNAMIC client.c -o client3 -ldl

clean:
	rm -f client1 client2 client3 collatz.o libsharedlib.so staticlib.a


.PHONY: clean all
