CC       = gcc
CFLAGS   = -Wall -std=c17 -g
TARGET   = main

USERS    = 5
PRINTERS = 3

.PHONY: all setup run clean

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) -o $(TARGET) main.c

setup:
	./$(TARGET) setup

run:
	./$(TARGET) run $(USERS) $(PRINTERS)

clean:
	./$(TARGET) cleanup || true
	-rm -f $(TARGET) shmfile semfile
