#ifndef CCEH_H_
#include "CCEH.h"
#endif

#include <iostream>
#include <thread>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <fstream>
#include <time.h>

#ifndef CONFIG_SCAS_COUNTER
#define CONFIG_SCAS_COUNTER
#endif

#define NR_OPERATIONS   1600000    // the data you generated must be greater or equal to this
#define INPUT_FILE      "data"      // using input_gen.cpp to generate data file

#define NR_THREADS      1        // must be equal or less nr_cpus

double *times;
unsigned long *directory_size;

uint64_t kWriteLatencyInNS = 0;
uint64_t clflushCount = 0;

double mysecond() {
    struct timeval tp;
    int i;
    i = gettimeofday(&tp, NULL);
    return ( (double)tp.tv_sec + (double)tp.tv_usec*1.e-6);
}

void print_result( CCEH *cceh) {
    double total_time = times[1] - times[0];
#ifdef INPLACE
    std::cout<< ", mode: INPLACE";
#else
    std::cout << ", mode: COW";
#endif
    std::cout << ", total time: " << total_time << \
    ", OPS: " << NR_OPERATIONS*1.0/(total_time) << \
    ", directory occupied space: " << cceh->StatisticGetDirectoryOccupiedSpace() << \
    ", directory doubling count: " << cceh->dir_doubling_cnt_ << \
    ", breakdown seg split: " << cceh->breakdown_seg_split_ << \
    ", breakdown maintenance dir: " << cceh->breakdown_maintenance_dir_ << \
    ", clflush segment count: " << clflush_seg_cnt_ << \
    ", clflush directory count: " << clflush_dir_cnt_ << \
     ", segment split num: " << cceh->split_num_<< \
    ", load factor: " << cceh->Utilization() <<std::endl;
}

int main() {

    // register_printcs_with_signal(SIGSEGV);
    // get keys and values from file
#ifdef USE_AEP
    init_vmp();
#else
#ifdef USE_EASTLAKE
    init_eastlake("a4", sizeof(Segment));
#endif
#endif
    Key_t *keys = new Key_t[NR_OPERATIONS];
    Value_t *values = new Value_t[NR_OPERATIONS];
    std::ifstream ifs;
    ifs.open(INPUT_FILE);
    unsigned long tmp;
    for(int i=0; i<NR_OPERATIONS; ++i) {
        ifs >> keys[i];
        ifs >> tmp;
        values[i] = (Value_t)tmp;
        //std::cout << "key: " << keys[i] << std::endl;
        //std::cout << "value: " << (unsigned long)values[i] << std::endl;
    }

    times = new double [2];
    CCEH *cceh;
     //test insert
   //test insert
     std::cout << "test insert: " << std::endl;
    cceh = new CCEH(2);
        times[0] = mysecond();
        for(int i=0; i<NR_OPERATIONS; i++) {
        cceh->Insert(keys[i], values[i]);
    }
        times[1] = mysecond();
        print_result(cceh);
        //delete cceh;
    

    // test get
    std::cout << "test get: " << std::endl;
     cceh = new CCEH(2);
     Value_t tmp_value;
     for(int i=0; i<NR_OPERATIONS; i++) {
        cceh->Insert(keys[i], values[i]);
    }
     times[0] = mysecond();
     for(int i=0; i<NR_OPERATIONS; i++) {
        tmp_value = cceh->Get(keys[i]);
        //  if(__glibc_unlikely(tmp_value != values[i])){
        //     std::cout << "get failed, key: " << (unsigned long)keys[i] << \
        //     " value: " << (unsigned long)values[i] << \
        //     " wrong value: " << (unsigned long)tmp_value << std::endl;
        // }
     }
     times[1] = mysecond();
    print_result(cceh);
    //delete cceh;
#ifdef USE_EASTLAKE
    destroy_eastlake("a4");
#endif
    return 0; 
}
