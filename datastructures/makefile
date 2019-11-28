.PHONY: clean run-all-ut run-all-test 

DEFAULT: test
# Define the tools we are going to use
CC= g++
LD = g++

# LIBS = -lm -lpthread -lrt -latomic -lvmem
LIBS = -lm -lpthread -lrt -latomic
CFLAGS = -g -Wall -Wno-unused -Wno-sign-compare -std=c++17 -I. -I../include -O3
OBJDIR = .

OBJS += $(OBJDIR)/CCEH_MSB.o 


JUNK = *~ $(OBJDIR)/*.o $(OBJDIR)/*.a $(OBJDIR)/*_ut  *-test

# CCEH-MSB-test:  CCEH_MSB_test.o aep.o CCEH_MSB.o slab.o
CCEH-MSB-test:  CCEH_MSB_test.o CCEH_MSB.o slab.o
	$(LD) $(CFLAGS) -o $@ $^ $(LIBS) 

CCEH_MSB_test.o: CCEH_MSB_test.cc
	$(CC) $(CFLAGS) -c -o $@ $<

CCEH_MSB.o: CCEH_MSB.cpp
	$(CC) $(CFLAGS) -c -o $@ $<
# aep.o: aep.cc
# 	$(CC) $(CFLAGS) -c -o $@ $<
slab.o: slab.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f  test $(JUNK)
