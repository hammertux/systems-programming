###################### DEFS

CC = gcc
CFLAGS = -Wall -pedantic -Werror
LDFLAGS = -ldl
DEPS = $(wildcard *.h)

###################### HELPERS

%.so : %.c
	$(CC) $(CFLAGS) -shared -nostdlib -fPIC -g $+ -o $@

%.o : %.c
	${CC} ${CFLAGS} -c -g $^ -o $@

####################### TARGETS

# add your libraries to this line, as in 'libtest.so', make sure you have a 'libtest.c' as source
LIBS = libblank.so

.PHONY : all clean distclean

all : client server ${LIBS}

client : client.o audio.o packet.o reverselib.o
	${CC} ${CFLAGS} -o $@ $+

server : server.o audio.o packet.o reverselib.o
	${CC} ${CFLAGS} -o $@ $+

distclean : clean
	rm -f server client *.so
clean:
	rm -f $(OBJECTS) server client *.o *.so *~

