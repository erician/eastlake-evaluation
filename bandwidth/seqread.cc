#ifndef EASTLAKE_H_
#include "eastlake.h"
#endif

#include <errno.h>
#include <sys/time.h>
#include <thread>
#include <string.h>

#define PO_NAME             "seqwrite"
#define PO_SIZE             (16*1024*1024*1024L)
#define EXTEND_MAP_SIZE     (1*1024*1024*1024L)
#define BLOCK_SIZE          (4096L)
#define BLOCK_NUM           (EXTEND_MAP_SIZE/BLOCK_SIZE)
#define NR_THREAD           (16L)


// #define PO_SIZE             (EACH_THREAD_RW_SIZE * NR_THREAD)

// #define EACH_THREAD_RW_SIZE (1024*1024*1024UL)


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
    for (int i=0; i<PO_SIZE/EXTEND_MAP_SIZE; i++) {
        if (po_extend(pod, EXTEND_MAP_SIZE, \
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

void SeqReadThread(int pod, int thread_id, int all_threads_num) {
    long each_thread_rw_extend_num = PO_SIZE/EXTEND_MAP_SIZE/all_threads_num;
    char *addrs[each_thread_rw_extend_num];
    for(long i=0; i<each_thread_rw_extend_num; i++) {
        addrs[i] = (char *)po_mmap(0, EXTEND_MAP_SIZE, PROT_READ|PROT_WRITE, \
            MAP_PRIVATE, pod, each_thread_rw_extend_num*thread_id*EXTEND_MAP_SIZE+i*EXTEND_MAP_SIZE);
        if (addrs[i] == NULL || addrs[i] < 0) {
            printf("po_mmap failed: %ld\n", (long)addrs[i]);
            exit(-1);
        }
    }
    char *buff = (char *)malloc(sizeof(char) * BLOCK_SIZE);
    for(long i=0; i<each_thread_rw_extend_num; i++) {
        for (long j=0; j<BLOCK_NUM; j++) {
            memcpy(buff, addrs[i]+j*BLOCK_SIZE, BLOCK_SIZE);
        }
    }
}

int main() {
    Prepare();
    double start, end;
    int pod;
    std::thread *threads[NR_THREAD];
    for(int thread_num=0; thread_num<NR_THREAD; thread_num+=2) {
        start = my_second();
        pod = po_open(PO_NAME, O_CREAT|O_RDWR, 0);
        for(long long int i=0; i<=thread_num; i++)
            threads[i] = new std::thread(SeqReadThread, pod, i, thread_num+1);
        for(int i=0; i<=thread_num; i++)
            threads[i]->join();
        end = my_second();
        printf("thread number: %d, po size: %ld, time: %lf, bandwidth: %lf\n", \
            thread_num+1, PO_SIZE, end-start, PO_SIZE/(end-start)/1024/1024);
        po_close(pod);
    }

    Cleanup();
}