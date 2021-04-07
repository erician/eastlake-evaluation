ALL: run_before_crash
.PHONY: clean all

DEFAULT:
# Define the tools we are going to use
CC= g++
LD = g++

LIBS = -lm -lpthread -lrt -latomic
CFLAGS = -g -Wall -Wno-unused -Wno-return-type -Wno-sign-compare -std=c++17 -I. -O0
CFLAGS += -I../include

JUNK = *~ *.o *.a *_ut *-* test

run_before_crash: run_before_crash.o
	$(LD) $(CFLAGS) -o $@ $^ $(LIBS)
run_before_crash.o: run_before_crash.cc
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f  test $(JUNK) run_before_crash run_after_crash
