CC = gcc
CFLAGS = -Wall -std=c17 -g

all: server client test_zad

server:
	$(CC) $(CFLAGS) -o server server.c

client:
	$(CC) $(CFLAGS) -o client client.c

test_zad: server client
	./server & \
	sleep 1 && \
	./client & \
	sleep 1 && \
	./client

clean:
	rm -f server client

.PHONY: all clean test_chat
