#ifndef EASTLAKE_H_
#include "eastlake.h"
#endif

#include <errno.h>
#include <sys/time.h>
#include <thread>
#include <string.h>

#define PO_NAME                 "seqwrite"
// #define PO_SIZE              (16*1024*1024*1024L)
// #define EXTEND_MAP_SIZE      (1*1024*1024*1024L)
#define PO_SIZE                 (160*1024*1024L)
#define EXTEND_MAP_SIZE         (10*1024*1024L)
#define BLOCK_SIZE              (4096L)
#define BLOCK_NUM               (EXTEND_MAP_SIZE/BLOCK_SIZE)
#define MAX_THREAD_NUM          (16L)

const int thread_num_cnt = 5;
const int thread_num[5] = {1, 2, 4, 8, 16};

long *access_pattern = NULL;

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
        char *addr = (char *)po_extend(pod, EXTEND_MAP_SIZE, PROT_READ|PROT_WRITE, MAP_PRIVATE);
        if (addr == NULL || addr < 0) {
            printf("Prepare po_extend, errno: %d\n", errno);
            exit(-1);
        } else {
            for(long j = 0; j < EXTEND_MAP_SIZE; j++)
                addr[j] = j;
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
        // printf("%ld, %ld, %ld\n", each_thread_rw_extend_num, thread_id, each_thread_rw_extend_num*thread_id*EXTEND_MAP_SIZE+i*EXTEND_MAP_SIZE);
    }
    char *buff = (char *)malloc(sizeof(char) * BLOCK_SIZE);
    for(long i=0; i<each_thread_rw_extend_num; i++) {
        for (long j=0; j<BLOCK_NUM; j++) {
            memcpy(buff, addrs[i]+j*BLOCK_SIZE, BLOCK_SIZE);
        }
    }
}

void SeqRead() {
    double start, end;
    int pod;
    std::thread *threads[MAX_THREAD_NUM];
    for(int t=0; t<thread_num_cnt; t+=1) {
        start = my_second();
        pod = po_open(PO_NAME, O_CREAT|O_RDWR, 0);
        for(long long int i=0; i<thread_num[t]; i++)
            threads[i] = new std::thread(SeqReadThread, pod, i, thread_num[t]);
        for(int i=0; i<thread_num[t]; i++)
            threads[i]->join();
        end = my_second();
        printf("thread number: %d, po size: %ld, time: %lf, bandwidth: %lf\n", \
            thread_num[t], PO_SIZE, end-start, PO_SIZE/(end-start)/1024/1024);
        po_close(pod);
    }
}

void SeqWriteThread(int pod, int thread_id, int all_threads_num) {
    long each_thread_rw_extend_num = PO_SIZE/EXTEND_MAP_SIZE/all_threads_num;
    char *addrs[each_thread_rw_extend_num];
    for(long i=0; i<each_thread_rw_extend_num; i++) {
        addrs[i] = (char *)po_mmap(0, EXTEND_MAP_SIZE, PROT_READ|PROT_WRITE, \
            MAP_PRIVATE, pod, each_thread_rw_extend_num*thread_id*EXTEND_MAP_SIZE+i*EXTEND_MAP_SIZE);
        if (addrs[i] == NULL || addrs[i] < 0) {
            printf("po_mmap failed: %ld\n", (long)addrs[i]);
            exit(-1);
        }
        //printf("%ld, %ld, %ld\n", each_thread_rw_extend_num, thread_id, each_thread_rw_extend_num*thread_id*EXTEND_MAP_SIZE+i*EXTEND_MAP_SIZE);
    }
    char *buff = (char *)malloc(sizeof(char) * BLOCK_SIZE);
    memset(buff, 'a', BLOCK_SIZE);
    for(long i=0; i<each_thread_rw_extend_num; i++) {
        for (long j=0; j<BLOCK_NUM; j++) {
            memcpy(addrs[i]+j*BLOCK_SIZE, buff, BLOCK_SIZE);
        }
    }
}

void SeqWrite() {
    double start, end;
    int pod;
    std::thread *threads[MAX_THREAD_NUM];
    for(int t=0; t<thread_num_cnt; t+=1) {
        start = my_second();
        pod = po_open(PO_NAME, O_CREAT|O_RDWR, 0);
        for(long long int i=0; i<thread_num[t]; i++)
            threads[i] = new std::thread(SeqWriteThread, pod, i, thread_num[t]);
        for(int i=0; i<thread_num[t]; i++)
            threads[i]->join();
        end = my_second();
        printf("thread number: %d, po size: %ld, time: %lf, bandwidth: %lf\n", \
            thread_num[t], PO_SIZE, end-start, PO_SIZE/(end-start)/1024/1024);
        po_close(pod);
    }
}

void RandReadThread(int pod, int thread_id, int all_threads_num) {
    long each_thread_rw_extend_num = PO_SIZE/EXTEND_MAP_SIZE/all_threads_num;
    char *addrs[each_thread_rw_extend_num];
    for(long i=0; i<each_thread_rw_extend_num; i++) {
        addrs[i] = (char *)po_mmap(0, EXTEND_MAP_SIZE, PROT_READ|PROT_WRITE, \
            MAP_PRIVATE, pod, each_thread_rw_extend_num*thread_id*EXTEND_MAP_SIZE+i*EXTEND_MAP_SIZE);
        if (addrs[i] == NULL || addrs[i] < 0) {
            printf("po_mmap failed: %ld\n", (long)addrs[i]);
            exit(-1);
        }
        // printf("%ld, %ld, %ld\n", each_thread_rw_extend_num, thread_id, each_thread_rw_extend_num*thread_id*EXTEND_MAP_SIZE+i*EXTEND_MAP_SIZE);
    }
    char *buff = (char *)malloc(sizeof(char) * BLOCK_SIZE);
    for(long i=0; i<each_thread_rw_extend_num; i++) {
        for (long j=0; j<BLOCK_NUM; j++) {
            memcpy(buff, addrs[i] + access_pattern[j]*BLOCK_SIZE, BLOCK_SIZE);
        }
    }
}

void RandRead() {
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
        printf("thread number: %d, po size: %ld, time: %lf, bandwidth: %lf\n", \
            thread_num[t], PO_SIZE, end-start, PO_SIZE/(end-start)/1024/1024);
        po_close(pod);
    }
}

void RandWriteThread(int pod, int thread_id, int all_threads_num) {
    long each_thread_rw_extend_num = PO_SIZE/EXTEND_MAP_SIZE/all_threads_num;
    char *addrs[each_thread_rw_extend_num];
    for(long i=0; i<each_thread_rw_extend_num; i++) {
        addrs[i] = (char *)po_mmap(0, EXTEND_MAP_SIZE, PROT_READ|PROT_WRITE, \
            MAP_PRIVATE, pod, each_thread_rw_extend_num*thread_id*EXTEND_MAP_SIZE+i*EXTEND_MAP_SIZE);
        if (addrs[i] == NULL || addrs[i] < 0) {
            printf("po_mmap failed: %ld\n", (long)addrs[i]);
            exit(-1);
        }
        // printf("%ld, %ld, %ld\n", each_thread_rw_extend_num, thread_id, each_thread_rw_extend_num*thread_id*EXTEND_MAP_SIZE+i*EXTEND_MAP_SIZE);
    }
    char *buff = (char *)malloc(sizeof(char) * BLOCK_SIZE);
    memset(buff, 'a', BLOCK_SIZE);
    for(long i=0; i<each_thread_rw_extend_num; i++) {
        for (long j=0; j<BLOCK_NUM; j++) {
            memcpy(addrs[i] + access_pattern[j]*BLOCK_SIZE, buff, BLOCK_SIZE);
        }
    }
}

void RandWrite() {
    double start, end;
    int pod;
    std::thread *threads[MAX_THREAD_NUM];
    for(int t=0; t<thread_num_cnt; t+=1) {
        start = my_second();
        pod = po_open(PO_NAME, O_CREAT|O_RDWR, 0);
        for(long long int i=0; i<thread_num[t]; i++)
            threads[i] = new std::thread(RandWriteThread, pod, i, thread_num[t]);
        for(int i=0; i<thread_num[t]; i++)
            threads[i]->join();
        end = my_second();
        printf("thread number: %d, po size: %ld, time: %lf, bandwidth: %lf\n", \
            thread_num[t], PO_SIZE, end-start, PO_SIZE/(end-start)/1024/1024);
        po_close(pod);
    }
}

int main() {
    Prepare();
    
    printf("SeqRead: \n");
    SeqRead();
    printf("SeqWrite: \n");
    SeqWrite();
    // prepare access pattern
    srand(time(NULL));
    access_pattern = (long *)malloc(sizeof(long) * BLOCK_NUM);
    for(long i = 0; i < BLOCK_NUM; i++) {
        access_pattern[i] = rand()%(BLOCK_NUM);
	}
    printf("RandRead: \n");
    RandRead();
    printf("RandWrite: \n");
    RandWrite();

    Cleanup();
}