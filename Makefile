#!/bin/bash

#GASNETDIR=${HOME}/degas/nwcEtr.git/src/tools/install/CRAYXC
GASNETDIR=${HOME}/degas/nwchemE.git/src/tools/install/CRAYXC

CC=cc
CXX=CC

CFLAGS = -std=c99 -g -O2

INCFLAGS = -I$(GASNETDIR)/include -I$(GASNETDIR)/include/aries-conduit
LDFLAGS = -L$(GASNETDIR)/lib
LIBS = -lgasnet-aries-seq -Wl,--whole-archive,-lhugetlbfs,--no-whole-archive


BINARY01=bench01
OBJS01=bench01.o
BINARY02=bench02
OBJS02=bench02.o

all: $(BINARY01) $(BINARY02)

$(BINARY01): $(OBJS01)
	$(CC) -o $(BINARY01) $^ $(CFLAGS) $(LDFLAGS) $(LIBS)

$(BINARY02): $(OBJS02)
	$(CC) -o $(BINARY02) $^ $(CFLAGS) $(LDFLAGS) $(LIBS)


%.o: %.c
	$(CC) -c $^ -o $@ $(CFLAGS) $(INCFLAGS)

clean:
	rm -f $(BINARY01) $(BINARY02) $(OBJS01) $(OBJS02)
