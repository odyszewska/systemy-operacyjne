CC = gcc
CFLAGS = -Wall -std=c17 -g

all: zad1  test_zad1 catcher sender test_zad2

zad1:
	$(CC) $(CFLAGS) -o zad1 zad1.c

sender:
	$(CC) $(CFLAGS) -o sender sender.c

catcher:
	$(CC) $(CFLAGS) -o catcher catcher.c

test_zad1:
	./zad1 0.0000001 5
	./zad1 0.000001 8
	./zad1 0.0000003 7
	./zad1 0.00001 9

test_zad2:
	./catcher &
	sleep 1
	echo "0 1" | ./sender

clean:
	rm zad1 catcher sender

.PHONY: clean all