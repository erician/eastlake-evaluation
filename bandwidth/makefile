.PHONY: clean

DEFAULT: randwrite randread seqread seqwrite
# Define the tools we are going to use
CC= g++
LD = g++

LIBS = -lm -lpthread -lrt -latomic
CFLAGS = -g -Wall -Wno-unused -Wno-return-type -Wno-sign-compare -std=c++17 -I.
CFLAGS += -I../include

JUNK = *~ *.o *.a *_ut *-* test

bw: bw.o
	$(LD) $(CFLAGS) -o $@ $^ $(LIBS) 
bw.o: bw.cc
	$(CC) $(CFLAGS) -c -o $@ $<

randwrite: randwrite.o
	$(LD) $(CFLAGS) -o $@ $^ $(LIBS) 
randwrite.o: randwrite.cc
	$(CC) $(CFLAGS) -c -o $@ $<

randread: randread.o
	$(LD) $(CFLAGS) -o $@ $^ $(LIBS) 
randread.o: randread.cc
	$(CC) $(CFLAGS) -c -o $@ $<

seqread: seqread.o
	$(LD) $(CFLAGS) -o $@ $^ $(LIBS) 
seqread.o: seqread.cc
	$(CC) $(CFLAGS) -c -o $@ $<

seqwrite: seqwrite.o
	$(LD) $(CFLAGS) -o $@ $^ $(LIBS) 
seqwrite.o: seqwrite.cc
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f  test $(JUNK) seqwrite seqread randwrite randread bw lat_bw
