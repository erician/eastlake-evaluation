all: seqread seqwrite prepare
.PHONY: clean all

DEFAULT: test
# Define the tools we are going to use
CC= g++
LD = g++

LIBS = -lm -lpthread -lrt -latomic
CFLAGS = -g -Wall -Wno-unused -Wno-return-type -Wno-sign-compare -std=c++17 -I.
CFLAGS += -I../include

JUNK = *~ *.o *.a *_ut *-*

seqread: seqread.o
	$(LD) $(CFLAGS) -o $@ $^ $(LIBS) 
seqread.o: seqread.cc
	$(CC) $(CFLAGS) -c -o $@ $<

seqwrite: seqwrite.o
	$(LD) $(CFLAGS) -o $@ $^ $(LIBS) 
seqwrite.o: seqwrite.cc
	$(CC) $(CFLAGS) -c -o $@ $<

prepare: prepare.o
	$(LD) $(CFLAGS) -o $@ $^ $(LIBS) 
prepare.o: prepare.cc
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(JUNK) seqwrite seqread prepare