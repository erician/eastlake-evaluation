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
#include <xmmintrin.h>

#ifndef EASTLAKE_H_
#include "eastlake.h"
#endif

#ifndef STREAM_TYPE
	#define STREAM_TYPE char
#endif

#ifndef MEM_SIZE
        // #define MEM_SIZE (16L * 1024 * 1024 * 1024)
	#define MEM_SIZE (160L * 1024 * 1024)
#endif 

#define TEST_EASTLAKE
#define NR_THREAD	16

int THREAD_NUM = 0;
pthread_t tids[NR_THREAD];
uint64_t BLOCK_SIZE = 0;
uint64_t BLOCK_NUM = 0;
STREAM_TYPE *pmem = NULL;
double start, end;
long long each_thread_access_num = 0;

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

void *read_thread(void *start) {
        char* buffer = (char*)malloc(BLOCK_SIZE);
        // init buffer
	memset(buffer, 0, BLOCK_SIZE);
        for (int i = 0; i < each_thread_access_num; i++) {
                memcpy(buffer, ((STREAM_TYPE **)start)[i], BLOCK_SIZE);
        }
        free(buffer);
}

void *write_thread(void *start) {
        char* buffer = (char*)malloc(BLOCK_SIZE);
        // init buffer
	memset(buffer, 0, BLOCK_SIZE);
        for (int i = 0; i < each_thread_access_num; i++) {
                for (int j=0; j<BLOCK_SIZE; j+=8) {
                        // use non-temporal stores, please see:
                        // https://software.intel.com/sites/landingpage/IntrinsicsGuide/#text=MOVNTi&expand=5667,5675
                        _mm_stream_si64((long long *)(((STREAM_TYPE **)start)[i] + j), (-1L));
                        // general stores
                        // *(long long *)(((STREAM_TYPE **)start)[i] + j) = (-1L);
                        // check data
                        // if (*(long long *)(((STREAM_TYPE **)start)[i] + j) != (-1L)) {
                        //         printf("error when assign value\n");
                        //         exit(-1);
                        // }
                }
                // use memcpy
                // memcpy(((STREAM_TYPE **)start)[i], buffer, BLOCK_SIZE);
        }
        free(buffer);
}

void
seqread() {
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
                        for (long i=0; i<thread_num+1; i++) {
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
                        printf("THREAD_NUM: %d BLOCK_SIZE:%lu time: %fs bw:%fMB/s latency:%fns\n", thread_num+1, BLOCK_SIZE, end-start, \
                                BLOCK_NUM*BLOCK_SIZE*sizeof(STREAM_TYPE)/(end-start)/1024/1024, (end-start)*1.e9/BLOCK_NUM);
                }
                free(access_list);
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
        char *po_name = "po_name";
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
	
	// randread
	printf("start randread()\n");
	randread();

	// randwrite
	printf("start randwrite()\n");
	randwrite();

#ifdef TEST_EASTLAKE
        po_close(pod);
        po_unlink(po_name);
#endif
	return 0;
}
