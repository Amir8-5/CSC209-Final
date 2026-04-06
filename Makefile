PORT=4242
CFLAGS= -Wall -DPORT=$(PORT)

all: server worker

server: server.c protocol.h
	gcc $(CFLAGS) -o server server.c

worker: client.c protocol.h
	gcc $(CFLAGS) -o client client.c

clean:
	rm -f server client