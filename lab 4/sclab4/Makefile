CC=gcc
CFLAGS=-Wall -W -g2
LDFLAGS=-g2
SOURCES=dv-network.c node.c
INCLUDES=node.h

default: all

all: sim

sim: ${SOURCES} ${INCLUDES}
	${CC} ${CFLAGS} ${LDFAGS} ${SOURCES} -o $@

clean:
	@rm -f sim
