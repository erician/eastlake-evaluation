#ifndef EASTLAKE_H_
#include "eastlake.h"
#endif

#include <errno.h>
#include <sys/time.h>
#include <thread>
#include <string.h>
#include <sched.h>
#include <mutex>
#include <emmintrin.h>

#define PO_NAME             "numa_aware_seqwrite"
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

void NTwrite(void* dst, void* src, uint64_t size) {
    uint64_t i;
    long long * ptr = (long long*)dst;
    long long * psrc = (long long*)src;
    for (i = 0; i < size/8; i++) {
        _mm_stream_si64(ptr + i, *(psrc + i));
    }
}

void Prepare(int pod, int thread_id, int all_threads_num) {
    set_affinity(thread_id);
    g_mutex.lock();

    char *addr = (char *)po_extend(pod, EXTEND_MAP_SIZE, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_NUMA_AWARE);
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

void SeqWriteThread(int pod, int thread_id, int all_threads_num) {
    set_affinity(thread_id);
    char *buff = (char *)malloc(sizeof(char) * BLOCK_SIZE);
    memset(buff, 'a', BLOCK_SIZE);
    
    // for (long i=0; i<BLOCK_NUM; i++) {
    //     memcpy(addrs[thread_id]+i*BLOCK_SIZE, buff, BLOCK_SIZE);
    // }
    for (long i=0; i<BLOCK_NUM; i++) {
        NTwrite(addrs[thread_id]+i*BLOCK_SIZE, buff, BLOCK_SIZE);
    }
}

void sandbox_check() {
    for (int i = 0; i < PO_SIZE/EXTEND_MAP_SIZE; i++){
        for (int j = 0; j < EXTEND_MAP_SIZE; j++) {
            if (*(addrs[i] + j) != 'a') {
                printf("error!\n");
                exit(-1);
            }
        }
    }
}

int main() {
    double start, end;
    int pod;
    std::thread *threads[MAX_THREAD_NUM];

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
            threads[i] = new std::thread(SeqWriteThread, pod, i, thread_num[t]);
        for(int i=0; i<thread_num[t]; i++)
            threads[i]->join();
        end = my_second();
        printf("thread number: %d, po size: %ld(MB), time: %lf(s), bandwidth: %lf(MB)\n", \
            thread_num[t], (EXTEND_MAP_SIZE*thread_num[t])/1024/1024, end-start, (EXTEND_MAP_SIZE*thread_num[t])/(end-start)/1024/1024);
        po_close(pod);
    }
    // sandbox_check();
    Cleanup();
}