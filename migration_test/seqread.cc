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
#include <unistd.h>

#define PO_NAME             "migration_seqread"
// #define PO_SIZE             (16*1024*1024*1024L)
// #define EXTEND_MAP_SIZE     (1*1024*1024*1024L)
#define PO_SIZE             (1600*1024*1024L)
#define EXTEND_MAP_SIZE     (100*1024*1024L)
#define BLOCK_SIZE          (4096L)
#define BLOCK_NUM           (EXTEND_MAP_SIZE/BLOCK_SIZE)
#define MAX_THREAD_NUM           (16L)
#define MAP_NUMA_AWARE         0x400000

// the size relative to the DRAM capacity. make the system to extend
#define PREPARE_SIZE    (186 * 1024 * 1024 * 1024L)
#define PREPARE_EXTEND_SIZE     (1*1024*1024*1024L)

// the size we will access
#define ACCESS_SIZE     (128*1024*1024L)
#define ACCESS_BLOCK_SIZE          (4096L)
#define ACCESS_BLOCK_NUM           (ACCESS_SIZE/ACCESS_BLOCK_SIZE)
#define LOOP_NUM    (500)

// const int thread_num_cnt = 5;
// const int thread_num[5] = {1, 2, 4, 8, 16};
const int thread_num_cnt = 1;
const int thread_num[5] = {1};

// record chunk addr
// char *addrs[PO_SIZE/EXTEND_MAP_SIZE];
char *prepare_addrs[PREPARE_SIZE/PREPARE_EXTEND_SIZE];
char *addr;
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


void Prepare() {
    set_affinity(0);
    // g_mutex.lock();

    for (long i = 0; i < PREPARE_SIZE/PREPARE_EXTEND_SIZE; i++) {
        prepare_addrs[i] = (char*)malloc(PREPARE_EXTEND_SIZE);
        if (prepare_addrs[i] == NULL) {
            printf("malloc error\n");
            exit(-1);
        }
        memset(prepare_addrs[i], 0, PREPARE_EXTEND_SIZE);
    }

    
    // g_mutex.unlock();
}

void Prepare_data() {
    set_affinity(0);
    addr = (char *)malloc(sizeof(char) * ACCESS_SIZE);
    memset(addr, 0, ACCESS_SIZE);
}

void Free() {
    for (long i = 0; i < PREPARE_SIZE/PREPARE_EXTEND_SIZE; i++) {
        free(prepare_addrs[i]);
    }
}

void Cleanup() {
    printf("start to Cleanup\n");
    free(addr);
    printf("Cleanup successfully\n");
}

void SeqReadThread() {
    char *buff = (char *)malloc(sizeof(char) * ACCESS_BLOCK_SIZE);

    for (long loop = 0; loop < LOOP_NUM; loop++) {
        for (long i = 0; i < ACCESS_BLOCK_NUM; i++) {
            memcpy(buff, addr + i * ACCESS_BLOCK_SIZE, ACCESS_BLOCK_SIZE);
        }
    }
    free(buff);
}
    

int main() {
    double start, end;
    int pod;
    std::thread *threads[MAX_THREAD_NUM];

    printf("start to Prepare\n");
    Prepare_data();
    printf("Prepare Over\n");

    for(int t=0; t<thread_num_cnt; t+=1) {
        start = my_second();
        SeqReadThread();
        end = my_second();
        printf("time: %lf(s), bandwidth: %lf(MB)\n", end-start, (ACCESS_SIZE * LOOP_NUM)/(end-start)/1024/1024);
    }
    
    Cleanup();
}
