CC=gcc
CFLAGS=-Wall -pedantic -O
DEPS=$(wildcard *.h)
SRC=$(wildcard *.c)
OBJ=$(wildcard *.o)

ping: pingclient1 pingclient2 pingclient3 pingserver

pingclient1.o: pingclient1.c $(DEPS)
	$(CC) $(CFLAGS) -c pingclient1.c -o $@

pingclient2.o: pingclient2.c $(DEPS)
	$(CC) $(CFLAGS) -c pingclient2.c -o $@

pingclient3.o: pingclient3.c $(DEPS)
	$(CC) $(CFLAGS) -c pingclient3.c -o $@

pingserver.o: pingserver.c $(DEPS)
	$(CC) $(CFLAGS) -c pingserver.c -o $@

pingclient1: pingclient1.o
	$(CC) -o $@ $^ $(CFLAGS)

pingclient2: pingclient2.o
	$(CC) -o $@ $^ $(CFLAGS)

pingclient3: pingclient3.o
	$(CC) -o $@ $^ $(CFLAGS)

pingserver: pingserver.o
	$(CC) -o $@ $^ $(CFLAGS)


.PHONY: clean ping


rmobj:
	rm -f $(OBJ)

clean: rmobj
	rm -f pingclient1 pingclient2 pingclient3 pingserver
