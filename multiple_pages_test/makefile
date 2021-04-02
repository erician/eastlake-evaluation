ALL: randwrite randread randwrite_mul randread_mul
.PHONY: clean

DEFAULT:
# Define the tools we are going to use
CC= g++
LD = g++

LIBS = -lm -lpthread -lrt -latomic
CFLAGS = -g -Wall -Wno-unused -Wno-return-type -Wno-sign-compare -std=c++17 -I.
CFLAGS += -I../include

JUNK = *~ *.o *.a *_ut *-* test

randwrite: randwrite.o
	$(LD) $(CFLAGS) -o $@ $^ $(LIBS) 
randwrite.o: randwrite.cc
	$(CC) $(CFLAGS) -c -o $@ $<

randread: randread.o
	$(LD) $(CFLAGS) -o $@ $^ $(LIBS) 
randread.o: randread.cc
	$(CC) $(CFLAGS) -c -o $@ $<

randwrite_mul: randwrite_mul.o
	$(LD) $(CFLAGS) -o $@ $^ $(LIBS) 
randwrite_mul.o: randwrite_mul.cc
	$(CC) $(CFLAGS) -c -o $@ $<

randread_mul: randread_mul.o
	$(LD) $(CFLAGS) -o $@ $^ $(LIBS) 
randread_mul.o: randread_mul.cc
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f  test $(JUNK) randwrite randread randwrite_mul randread_mul
