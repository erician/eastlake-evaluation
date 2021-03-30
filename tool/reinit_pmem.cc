#ifndef EASTLAKE_H_
#include "eastlake.h"
#endif

#include <errno.h>
#include <sys/time.h>
#include <thread>
#include <string.h>
#include <sched.h>
#include <mutex>

int main() {
    // creat the po
    pmem_init();
    printf("finished\n");
    return 0;
}