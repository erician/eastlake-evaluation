all: reinit_pmem
.PHONY: clean all

DEFAULT: test
# Define the tools we are going to use
CC= g++
LD = g++

LIBS = -lm -lpthread -lrt -latomic
CFLAGS = -g -Wall -Wno-unused -Wno-return-type -Wno-sign-compare -std=c++17 -I.
CFLAGS += -I../include

JUNK = *~ *.o *.a *_ut *-*

reinit_pmem: reinit_pmem.o
	$(LD) $(CFLAGS) -o $@ $^ $(LIBS) 
reinit_pmem.o: reinit_pmem.cc
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(JUNK) reinit_pmem
