#ifndef EASTLAKE_H_
#include "eastlake.h"
#endif

#include <errno.h>
#include <sys/time.h>
#include <thread>

#define PO_NAME     "seqread"
#define MAX_PO_EXTEND_SIZE  (256*1024*1024UL)
#define PO_SIZE     (MAX_PO_EXTEND_SIZE * NR_THREAD)
#define NR_THREAD   16

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

void SeqReadThread(int pod, int thread_num, int thread_id) {
    unsigned long long each_thread_read_size = PO_SIZE/MAX_PO_EXTEND_SIZE/thread_num;
    unsigned long *addr;
    unsigned long tmp;
    for(unsigned long long i=thread_id*each_thread_read_size; \
        i < ((thread_id+1)*each_thread_read_size); i++) {
        addr = (unsigned long *)po_mmap(0, MAX_PO_EXTEND_SIZE, \
            PROT_READ|PROT_WRITE, MAP_PRIVATE, pod, i*MAX_PO_EXTEND_SIZE);
        for (unsigned long j=0; j<MAX_PO_EXTEND_SIZE/(sizeof(unsigned long)); j++) {
            tmp = *(addr + j);
        }
    }
}

int main() {
    Prepare();

    double start, end;
    int pod;
    std::thread *threads[NR_THREAD];
    for(int thread_num=0; thread_num<NR_THREAD; thread_num++) {
        start = my_second();
        pod = po_open(PO_NAME, O_CREAT|O_RDWR, 0);
        for(long long int i=0; i<=thread_num; i++)
            threads[i] = new std::thread(SeqReadThread, pod, thread_num+1, i);
        for(int i=0; i<=thread_num; i++)
            threads[i]->join();
        end = my_second();
        po_close(pod);
        printf("thread number: %d, po size: %ld, time: %lf, bandwidth: %lf\n", \
            thread_num, PO_SIZE, end-start, PO_SIZE/(end-start)/1024/1024);
    }
    
    Cleanup();
}
