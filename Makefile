CC = gcc
CFLAGS = -Wall -g

all: server client

server: server.o
	$(CC) $(CFLAGS) -o server server.o

client: client.o
	$(CC) $(CFLAGS) -o client client.o

server.o: server.c protocol.h
	$(CC) $(CFLAGS) -c server.c

client.o: client.c protocol.h
	$(CC) $(CFLAGS) -c client.c

clean:
	rm -f *.o server client

