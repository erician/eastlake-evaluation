#ifndef EASTLAKE_H_
#include "eastlake.h"
#endif

#include <errno.h>
#include <sys/time.h>
#include <thread>
#include <string.h>

#define PO_NAME             "seqwrite1"
#define EACH_THREAD_RW_SIZE (2*1024*1024*1024UL)
#define BLOCK_SIZE          (4*1024)
#define BLOCK_NUM           (EACH_THREAD_RW_SIZE/BLOCK_SIZE)
#define NR_THREAD           16
#define PO_SIZE             (EACH_THREAD_RW_SIZE * NR_THREAD)

char *buff;

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
    for (int i=0; i<PO_SIZE/EACH_THREAD_RW_SIZE; i++) {
        if (po_extend(pod, EACH_THREAD_RW_SIZE, \
            PROT_READ|PROT_WRITE, MAP_PRIVATE) == 0) {
            printf("Prepare po_extend, errno: %d\n", errno);
            exit(-1);
        }
    }
    po_close(pod);
    buff = (char *)malloc(sizeof(char) * BLOCK_SIZE);
    memset(buff, 'a', BLOCK_SIZE);
}

void Cleanup() {
    printf("start to Cleanup\n");
    if (po_unlink(PO_NAME) != 0) {
        printf("Cleanup, unlink failed\n");
    }
    printf("Cleanup successfully\n");
}

void SeqWriteThread(int pod, int thread_id) {
    char *addr;
    unsigned long tmp;
    addr = (char *)po_mmap(0, EACH_THREAD_RW_SIZE, \
        PROT_READ|PROT_WRITE, MAP_PRIVATE, pod, thread_id*EACH_THREAD_RW_SIZE);
    if (addr == NULL || addr < 0) {
        printf("po_mmap failed: %ld\n", (long)addr);
        exit(-1);
    }
    for (int i=0; i<BLOCK_NUM; i++) {
        memcpy(addr+i*BLOCK_SIZE, buff, BLOCK_SIZE);
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
            threads[i] = new std::thread(SeqWriteThread, pod, i);
        for(int i=0; i<=thread_num; i++)
            threads[i]->join();
        end = my_second();
        printf("thread number: %d, po size: %ld, time: %lf, bandwidth: %lf\n", \
            thread_num+1, PO_SIZE, end-start, EACH_THREAD_RW_SIZE*(thread_num+1)/(end-start)/1024/1024);
        po_close(pod);
    }
    Cleanup();
}
