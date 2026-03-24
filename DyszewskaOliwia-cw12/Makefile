CC     = gcc
CFLAGS = -Wall -std=c17 -g
LIBS   = -pthread

TARGETS = chat_server chat_client

all: $(TARGETS)

chat_server: chat_server.c
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

chat_client: chat_client.c
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)


clean:
	rm -f $(TARGETS)

.PHONY: all clean
