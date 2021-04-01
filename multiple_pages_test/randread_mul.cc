#ifndef EASTLAKE_H_
#include "eastlake.h"
#endif

#include <errno.h>
#include <sys/time.h>
#include <thread>
#include <string.h>

#define PO_NAME             "randread"
// #define PO_SIZE             (16*1024*1024*1024L)
// #define EXTEND_MAP_SIZE     (1*1024*1024*1024L)
#define PO_SIZE             (1600*1024*1024L)
#define EXTEND_MAP_SIZE     (100*1024*1024L)
#define BLOCK_SIZE          (4096L)
#define BLOCK_NUM           (EXTEND_MAP_SIZE/BLOCK_SIZE)
#define MAX_THREAD_NUM           (16L)
#define MAP_NUMA_AWARE         0x400000

const int thread_num_cnt = 5;
const int thread_num[5] = {1, 2, 4, 8, 16};

long *access_pattern = NULL;
// record chunk addr
char *addrs[PO_SIZE/EXTEND_MAP_SIZE];

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
        char *addr = (char *)po_extend(pod, EXTEND_MAP_SIZE, PROT_READ|PROT_WRITE, MAP_PRIVATE | MAP_HUGETLB | MAP_NUMA_AWARE);
        if (addr == NULL || addr < 0) {
            printf("Prepare po_extend, errno: %d\n", errno);
            exit(-1);
        } else {
            for(long j = 0; j < EXTEND_MAP_SIZE; j++)
                addr[j] = j;
            addrs[i] = addr;
        }
    }
    po_close(pod);

    // prepare access pattern
    srand(time(NULL));
    access_pattern = (long *)malloc(sizeof(long) * BLOCK_NUM);
    for(long i = 0; i < BLOCK_NUM; i++) {
        access_pattern[i] = rand()%(BLOCK_NUM);
	}
}

void Cleanup() {
    printf("start to Cleanup\n");
    if (po_unlink(PO_NAME) != 0) {
        printf("Cleanup, unlink failed\n");
    }
    printf("Cleanup successfully\n");
}

void RandReadThread(int pod, int thread_id, int all_threads_num) {
    long each_thread_rw_extend_num = PO_SIZE/EXTEND_MAP_SIZE/all_threads_num;
    char *buff = (char *)malloc(sizeof(char) * BLOCK_SIZE);
    for(long i=0; i<each_thread_rw_extend_num; i++) {
        for (long j=0; j<BLOCK_NUM; j++) {
            memcpy(buff, addrs[each_thread_rw_extend_num*thread_id+i] + access_pattern[j]*BLOCK_SIZE, BLOCK_SIZE);
        }
    }
}

int main() {
    Prepare();
    double start, end;
    int pod;
    std::thread *threads[MAX_THREAD_NUM];
    for(int t=0; t<thread_num_cnt; t+=1) {
        start = my_second();
        pod = po_open(PO_NAME, O_CREAT|O_RDWR, 0);
        for(long long int i=0; i<thread_num[t]; i++)
            threads[i] = new std::thread(RandReadThread, pod, i, thread_num[t]);
        for(int i=0; i<thread_num[t]; i++)
            threads[i]->join();
        end = my_second();
        printf("thread number: %d, po size: %ld(MB), time: %lf(s), bandwidth: %lf(MB)\n", \
            thread_num[t], PO_SIZE/1024/1024, end-start, PO_SIZE/(end-start)/1024/1024);
        po_close(pod);
    }

    Cleanup();
}
