#ifndef EASTLAKE_H_
#include "eastlake.h"
#endif

#include <errno.h>
#include <sys/time.h>
#include <thread>
#include <string.h>
#include <sched.h>
#include <mutex>

#define PO_NAME             "numa_seqread"
// #define PO_SIZE             (16*1024*1024*1024L)
// #define EXTEND_MAP_SIZE     (1*1024*1024*1024L)
#define PO_SIZE             (1600*1024*1024L)
#define EXTEND_MAP_SIZE     (100*1024*1024L)
#define BLOCK_SIZE          (4096L)
#define BLOCK_NUM           (EXTEND_MAP_SIZE/BLOCK_SIZE)
#define NR_THREAD           (16L)
#define MAP_NUMA_AWARE         0x400000

const int thread_num_cnt = 5;
const int thread_num[5] = {1, 2, 4, 8, 16};

// record chunk addr
char *addrs[PO_SIZE/EXTEND_MAP_SIZE];
std::mutex g_mutex;

double my_second() {
    struct timeval tp;
    int i;
    i = gettimeofday(&tp, NULL);
    return ( (double)tp.tv_sec + (double)tp.tv_usec*1.e-6);
}

/* in our system
 * node0: 0-17, 36-53
 * node1: 18-35, 54-71
 * 16 threads : 0-15
 */
void set_affinity(uint32_t idx) {
    cpu_set_t my_set;
    CPU_ZERO(&my_set);
    if (idx % 2 != 0) {
        CPU_SET(idx + 18, &my_set);
    } else {
        CPU_SET(idx, &my_set);
    }
    sched_setaffinity(0, sizeof(cpu_set_t), &my_set);
}

void Prepare(int pod, int thread_id, int all_threads_num) {
    set_affinity(thread_id);
    g_mutex.lock();

    char *addr = (char *)po_extend(pod, EXTEND_MAP_SIZE, PROT_READ|PROT_WRITE, MAP_PRIVATE);
    if (addr == NULL || addr < 0) {
        printf("Prepare po_extend, errno: %d\n", errno);
        exit(-1);
    } else {
        for(long j = 0; j < EXTEND_MAP_SIZE; j++)
            addr[j] = j;
        addrs[thread_id] = addr;
    }
    g_mutex.unlock();
}

void Cleanup() {
    printf("start to Cleanup\n");
    if (po_unlink(PO_NAME) != 0) {
        printf("Cleanup, unlink failed\n");
    }
    printf("Cleanup successfully\n");
}

void SeqReadThread(int pod, int thread_id, int all_threads_num) {
    set_affinity(thread_id);
    // each one deal with one EXTEND_MAP_SIZE
    char *buff = (char *)malloc(sizeof(char) * BLOCK_SIZE);
    for (long i = 0; i < BLOCK_NUM; i++) {
        memcpy(buff, addrs[thread_id] + i * BLOCK_SIZE, BLOCK_SIZE);
    }
}



int main() {
    // creat the po
    double start, end;
    int pod;
    std::thread *threads[NR_THREAD];

    printf("start to Prepare\n");
    pod = po_creat(PO_NAME, 0);
    if (pod == -1) {
        printf("Prepare po_creat, errno: %d\n", errno);
        exit(-1);
    }
    for(long long int i=0; i<thread_num[thread_num_cnt-1]; i++)
        threads[i] = new std::thread(Prepare, pod, i, thread_num[thread_num_cnt-1]);
    for(int i=0; i<thread_num[thread_num_cnt-1]; i++)
        threads[i]->join();
    po_close(pod);
    printf("Prepare Over\n");
    
    for(int t=0; t<thread_num_cnt; t+=1) {
        start = my_second();
        pod = po_open(PO_NAME, O_CREAT|O_RDWR, 0);
        
        for(long long int i=0; i<thread_num[t]; i++)
            threads[i] = new std::thread(SeqReadThread, pod, i, thread_num[t]);
        for(int i=0; i<thread_num[t]; i++)
            threads[i]->join();
        end = my_second();
        printf("thread number: %d, po size: %ld(MB), time: %lf(s), bandwidth: %lf(MB)\n", \
            thread_num[t], (EXTEND_MAP_SIZE*thread_num[t])/1024/1024, end-start, (EXTEND_MAP_SIZE*thread_num[t])/(end-start)/1024/1024);
        po_close(pod);
    }

    Cleanup();
}