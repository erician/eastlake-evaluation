#ifndef EASTLAKE_H_
#include "eastlake.h"
#endif

#include <errno.h>
#include <sys/time.h>

#define PO_NAME     "seqread10"
#define MAX_PO_EXTEND_SIZE  (1024*4*1024UL)
#define PO_SIZE     (MAX_PO_EXTEND_SIZE * 100)
#define NR_THREAD   1

double my_second() {
    struct timeval tp;
    int i;
    i = gettimeofday(&tp, NULL);
    return ( (double)tp.tv_sec + (double)tp.tv_usec*1.e-6);
}

void Prepare() {
    printf("start to Prepare\n");
    int pod = po_creat(PO_NAME, 0);
    if (pod == -1) {
        printf("Prepare po_creat, errno: %d\n", errno);
        exit(-1);
    }
    for (int i=0; i<PO_SIZE/MAX_PO_EXTEND_SIZE; i++) {
        //printf("i: %d\n", i);
        if (po_extend(pod, MAX_PO_EXTEND_SIZE, \
            PROT_READ|PROT_WRITE, MAP_PRIVATE) == 0) {
            printf("Prepare po_extend, errno: %d\n", errno);
            exit(-1);
        }
    }
    po_close(pod);
}

void Cleanup() {
    printf("start to Cleanup\n");
    if (po_unlink(PO_NAME) != 0) {
        printf("Cleanup, unlink failed\n");
    }
    printf("Cleanup successfully\n");
}

void SeqRead() {
    printf("start to SeqRead\n");
    int pod;
    pod = po_open(PO_NAME, O_CREAT|O_RDWR, 0);

    unsigned long *addr;
    unsigned long tmp;
    for (unsigned long i=0; i<PO_SIZE/MAX_PO_EXTEND_SIZE; i++) {
        //printf("i: %d\n", i);
        addr = (unsigned long *)po_mmap(0, MAX_PO_EXTEND_SIZE, \
            PROT_READ|PROT_WRITE, MAP_PRIVATE, pod, i*MAX_PO_EXTEND_SIZE);
        for (unsigned long j=0; j<MAX_PO_EXTEND_SIZE/(sizeof(unsigned long)); j++) {
            tmp = *(addr + j);
        }
    }
    po_close(pod);
}

int main() {
    Prepare();
    double start = my_second();
    SeqRead();
    double end = my_second();
    printf("thread number: %d, po size: %ld, time: %lf, bandwidth: %lf\n", \
        NR_THREAD, PO_SIZE, end-start, PO_SIZE/(end-start));
    Cleanup();
}