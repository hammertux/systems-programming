###################### DEFS

CC = gcc
CFLAGS = -Wall -pedantic
LDFLAGS = -ldl
DEPS = $(wildcard *.h)

###################### HELPERS

%.so : %.c
	$(CC) $(CFLAGS) -shared -nostdlib -fPIC -g $+ -o $@

%.o : %.c
	${CC} ${CFLAGS}  -c -g $^ ${LDFLAGS} -o $@

####################### TARGETS

# add your libraries to this line, as in 'libtest.so', make sure you have a 'libtest.c' as source
LIBS = libspeed.so libmono.so libvolume.so

.PHONY : all clean distclean

all : client server ${LIBS}

client : client.o audio.o packet.o
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ $+

server : server.o audio.o packet.o
	${CC} ${CFLAGS}  -o $@ $+ ${LDFLAGS} -L.-lmono -L.-lvolume -L.-lspeed

distclean : clean
	rm -f server client *.so
clean:
	rm -f $(OBJECTS) server client *.o *.so *~

