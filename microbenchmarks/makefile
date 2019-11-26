.PHONY: clean ll-test

DEFAULT: test
# Define the tools we are going to use
CC= g++
LD = g++

LIBS = -lm -lpthread -lrt -latomic
CFLAGS = -g -Wall -Wno-unused -Wno-return-type -Wno-sign-compare -std=c++17 -I.
CFLAGS += -I../include

JUNK = *~ *.o *.a *_ut *-* test

test: test.o
	$(LD) $(CFLAGS) -o $@ $^ $(LIBS) 

test.o: test.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f  test $(JUNK) 
