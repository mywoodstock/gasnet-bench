#!/bin/bash

#GASNETDIR=${HOME}/degas/nwcEtr.git/src/tools/install/CRAYXC
GASNETDIR=${HOME}/degas/nwchemE.git/src/tools/install/CRAYXC

CC=cc
CXX=CC

CFLAGS = -std=c99 -g -O2

INCFLAGS = -I$(GASNETDIR)/include -I$(GASNETDIR)/include/aries-conduit
LDFLAGS = -L$(GASNETDIR)/lib
LIBS = -lgasnet-aries-seq -Wl,--whole-archive,-lhugetlbfs,--no-whole-archive


BINARY=bench01
OBJS=bench01.o


$(BINARY): $(OBJS)
	$(CC) -o $(BINARY) $^ $(CFLAGS) $(LDFLAGS) $(LIBS)

%.o: %.c
	$(CC) -c $^ -o $@ $(CFLAGS) $(INCFLAGS)

clean:
	rm -f $(BINARY) $(OBJS)
