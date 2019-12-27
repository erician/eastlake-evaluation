/*
	This is to test the latency and bandwidth of AEP.
        To use PMDK, visit: 
        https://software.intel.com/en-us/articles/quick-start-guide-configure-intel-optane-dc-persistent-memory-on-linux
*/
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <libpmem.h>
#include <sys/time.h>
#include <pthread.h>
#include <time.h>
#include <stdint.h>

#define TEST_EASTLAKE

#ifndef EASTLAKE_H_
#include "eastlake.h"
#endif

#ifndef STREAM_TYPE
	#define STREAM_TYPE char
#endif

#ifndef MEM_SIZE
	#define MEM_SIZE (16L * 1024 * 1024 * 1024)
#endif 

#define NR_THREAD	1

int THREAD_NUM = 0;
pthread_t tids[NR_THREAD];
uint64_t BLOCK_SIZE = 0;
uint64_t BLOCK_NUM = 0;
STREAM_TYPE *pmem = NULL;
double start, end;
long long each_thread_access_num = 0;
long long *access_pattern = NULL;

#ifdef TEST_EASTLAKE
int pod = 0;
#endif

// #define BLOCK_SIZE_LENGTH 11
// uint64_t BLOCK_SIZE_LIST[BLOCK_SIZE_LENGTH] = {16, 32, 64, 128, 256, 512, 
//                                                 1024, 2048, 4096, 8192, 16384,};
#define BLOCK_SIZE_LENGTH 1
uint64_t BLOCK_SIZE_LIST[BLOCK_SIZE_LENGTH] = {4096,};

double mysecond()
{
	struct timeval tp;

        gettimeofday(&tp, NULL);
        return ( (double)tp.tv_sec + (double)tp.tv_usec * 1.e-6 );
}

void *read_thread(void *tid) {
        STREAM_TYPE **start = (STREAM_TYPE **)po_mmap(0, each_thread_access_num, \
            PROT_READ|PROT_WRITE, MAP_PRIVATE, pod, ((long)tid)*each_thread_access_num);
        char* buffer = (char*)malloc(BLOCK_SIZE);
        // init buffer
	memset(buffer, 0, BLOCK_SIZE);
        for (int i = 0; i < each_thread_access_num; i++) {
                memcpy(buffer, start + access_pattern[i], BLOCK_SIZE);
        }
        free(buffer);
}

void *write_thread(void *start) {
        char* buffer = (char*)malloc(BLOCK_SIZE);
        // init buffer
	memset(buffer, 0, BLOCK_SIZE);
        for (int i = 0; i < each_thread_access_num; i++) {
                memcpy(((STREAM_TYPE **)start)[i], buffer, BLOCK_SIZE);
        }
        free(buffer);
}

void
seqread() {
	uint64_t i, j;
	
	for (i = 0; i < BLOCK_SIZE_LENGTH; i++) {
        	BLOCK_SIZE = BLOCK_SIZE_LIST[i];
                BLOCK_NUM = MEM_SIZE / BLOCK_SIZE;      
	
                for (int thread_num = 0; thread_num < NR_THREAD; thread_num++) {
                        each_thread_access_num = BLOCK_NUM/(thread_num+1);
                        access_pattern = (long long *)malloc(sizeof(long long)*each_thread_access_num);
                        for (int i=0; i<each_thread_access_num; i++)
                                access_pattern[i] = 0 + i*BLOCK_SIZE;
                        start = mysecond();
                        for (long i=0; i<thread_num+1; i++) {
                                if (pthread_create(&tids[i], NULL, read_thread, (void *)(i)) != 0) {
                                        printf("pthread_create failed\n");
                                        exit(-1);
                                }
                        }
                        end = mysecond();
                        printf("create thread time: %fs\n", end-start);
                        start = mysecond();
                        for (int i=0; i<thread_num+1; i++) {
                                pthread_join(tids[i], NULL);
                        }
                        end = mysecond();
                        free(access_pattern);
                        printf("THREAD_NUM: %d BLOCK_SIZE:%lu time: %fs bw:%fMB/s latency:%fns\n", thread_num+1, BLOCK_SIZE, end-start, \
                                BLOCK_NUM*BLOCK_SIZE*sizeof(STREAM_TYPE)/(end-start)/1024/1024, (end-start)*1.e9/BLOCK_NUM);
                }
        }
}

void seqwrite() {
	uint64_t i, j;
	for (i = 0; i < BLOCK_SIZE_LENGTH; i++) {
                BLOCK_SIZE = BLOCK_SIZE_LIST[i];
                BLOCK_NUM = MEM_SIZE / BLOCK_SIZE;

		//STREAM_TYPE* access_list[BLOCK_NUM];
                STREAM_TYPE** access_list = (STREAM_TYPE**)malloc(sizeof(STREAM_TYPE*) * BLOCK_NUM);
		// populate it with the addr
                for(j = 0; j < BLOCK_NUM; j++) {
                        access_list[j] = pmem + j * BLOCK_SIZE;
                }

                for (int thread_num = 0; thread_num < NR_THREAD; thread_num++) {
                        each_thread_access_num = BLOCK_NUM/(thread_num+1);
                        start = mysecond();
                        for (int i=0; i<thread_num+1; i++) {
                                if (pthread_create(&tids[i], NULL, write_thread, \
                                        (void *)(access_list+i*each_thread_access_num)) != 0) {
                                        printf("pthread_create failed\n");
                                        exit(-1);
                                }
                        }
                        end = mysecond();
                        printf("create thread time: %f\n", end-start);
                        for (int i=0; i<thread_num+1; i++) {
                                pthread_join(tids[i], NULL);
                        }
                        end = mysecond();
                        printf("THREAD_NUM: %d BLOCK_SIZE:%lu time: %fs bw:%fMB/s latency:%fns\n", thread_num+1, BLOCK_SIZE, end-start, \
                                BLOCK_NUM*BLOCK_SIZE*sizeof(STREAM_TYPE)/(end-start)/1024/1024, (end-start)*1.e9/BLOCK_NUM);
                }
                free(access_list);
        }
}

void randread() {
	uint64_t i, j;

	for (i = 0; i < BLOCK_SIZE_LENGTH; i++) {
                BLOCK_SIZE = BLOCK_SIZE_LIST[i];
                BLOCK_NUM = MEM_SIZE / BLOCK_SIZE;

		//STREAM_TYPE* access_list[BLOCK_NUM];
		STREAM_TYPE** access_list = (STREAM_TYPE**)malloc(sizeof(STREAM_TYPE*) * BLOCK_NUM);

		// populate it with the addr
		for(j = 0; j < BLOCK_NUM; j++) {
			access_list[j] = pmem + j * BLOCK_SIZE;
		} 
		
		// shuffer
		for(j = 0; j < BLOCK_NUM; j++) {
			uint64_t index = rand() % (BLOCK_NUM-j) + j; // j ~ BLOCK_NUM
			STREAM_TYPE *tmp = access_list[j];
			access_list[j] = access_list[index];
			access_list[index] = tmp;
		}
		for (int thread_num = 0; thread_num < NR_THREAD; thread_num++) {
                        each_thread_access_num = BLOCK_NUM/(thread_num+1);
                        start = mysecond();
                        for (int i=0; i<thread_num+1; i++) {
                                if (pthread_create(&tids[i], NULL, read_thread, \
                                        (void *)(access_list+i*each_thread_access_num)) != 0) {
                                        printf("pthread_create failed\n");
                                        exit(-1);
                                }
                        }
                        for (int i=0; i<thread_num+1; i++) {
                                pthread_join(tids[i], NULL);
                        }
                        end = mysecond();
                        printf("THREAD_NUM: %d BLOCK_SIZE:%lu bw:%fMB/s latency:%fns\n", thread_num+1, BLOCK_SIZE, \
                                BLOCK_NUM*BLOCK_SIZE*sizeof(STREAM_TYPE)/(end-start)/1024/1024, (end-start)*1.e9/BLOCK_NUM);
                }
                free(access_list);
        }
}

void randwrite() {
	uint64_t i, j;
        for (i = 0; i < BLOCK_SIZE_LENGTH; i++) {
                BLOCK_SIZE = BLOCK_SIZE_LIST[i];
                BLOCK_NUM = MEM_SIZE / BLOCK_SIZE;

		//STREAM_TYPE* access_list[BLOCK_NUM];
                STREAM_TYPE** access_list = (STREAM_TYPE**)malloc(sizeof(STREAM_TYPE*) * BLOCK_NUM);
		
		// populate it with the addr
                for(j = 0; j < BLOCK_NUM; j++) {
                        access_list[j] = pmem + j * BLOCK_SIZE;
                }

                // shuffer
                for(j = 0; j < BLOCK_NUM; j++) {
                        uint64_t index = rand() % (BLOCK_NUM-j) + j; // j ~ BLOCK_NUM
                        STREAM_TYPE *tmp = access_list[j];
                        access_list[j] = access_list[index];
                        access_list[index] = tmp;
                }
                
                for (int thread_num = 0; thread_num < NR_THREAD; thread_num++) {
                        each_thread_access_num = BLOCK_NUM/(thread_num+1);
                        start = mysecond();
                        for (int i=0; i<thread_num+1; i++) {
                                if (pthread_create(&tids[i], NULL, write_thread, \
                                        (void *)(access_list+i*each_thread_access_num)) != 0) {
                                        printf("pthread_create failed\n");
                                        exit(-1);
                                }
                        }
                        for (int i=0; i<thread_num+1; i++) {
                                pthread_join(tids[i], NULL);
                        }
                        end = mysecond();
                        printf("THREAD_NUM: %d BLOCK_SIZE:%lu bw:%fMB/s latency:%fns\n", thread_num+1, BLOCK_SIZE, \
                                BLOCK_NUM*BLOCK_SIZE*sizeof(STREAM_TYPE)/(end-start)/1024/1024, (end-start)*1.e9/BLOCK_NUM);
                }
                free(access_list);
        }
}

int main(void)
{
	char *path = "/home/tony/Desktop/pmemdir/a";
        char *po_name = "aaaaaaaaa";
	int is_pmem = 0;
	size_t mapped_len;	
	size_t i = 0;
	size_t j = 0;	
	//double start, end;

	srand(time(NULL));
	printf("start to map\n");
#ifndef TEST_EASTLAKE
	if ( (pmem = (STREAM_TYPE*)pmem_map_file(path, MEM_SIZE, 
			PMEM_FILE_CREATE, 0666, &mapped_len, &is_pmem)) == NULL )
	{
		perror("pmem_map_file");
		exit(1);
	}
        //pmem = (STREAM_TYPE*)malloc(MEM_SIZE);
	printf("pmem:%p is_pmem:%d\n", pmem, is_pmem);
#else
        pod = po_creat(po_name, 0);
        if (pod == -1) {
                printf("po_creat, errno: %d\n", errno);
                exit(-1);
        }
        if ((pmem = (STREAM_TYPE*)po_extend(pod, MEM_SIZE, \
                PROT_READ|PROT_WRITE, MAP_PRIVATE)) == 0) {
                printf("po_extend, errno: %d\n", errno);
                exit(-1);
        }
        printf("test eastlake\n");
#endif
	
        // print NR_THREAD
        printf("NR_THREAD: %d\n", NR_THREAD);
	// init
	for (i = 0; i < MEM_SIZE/(sizeof(STREAM_TYPE)); i++) {
		pmem[i] = 0x0;
	}
	// seqread
	printf("start seqread()\n");
	seqread();	

	// seqwrite
	printf("start seqwrite()\n");
	seqwrite();
	
	// // randread
	// printf("start randread()\n");
	// randread();

	// // randwrite
	// printf("start randwrite()\n");
	// randwrite();

#ifdef TEST_EASTLAKE
        po_close(pod);
        po_unlink(po_name);
#endif
	return 0;
}
