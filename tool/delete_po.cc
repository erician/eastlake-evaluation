#ifndef EASTLAKE_H_
#include "eastlake.h"
#endif

#include <errno.h>
#include <sys/time.h>
#include <thread>
#include <string.h>
#include <sched.h>
#include <mutex>

int main(int argc, char* argv[]) {
    // creat the po
    if (argc != 2) {
        printf("please input the po_name\n");
        return 0;
    }
    if (po_unlink(argv[1]) != 0) {
        printf("Cleanup, unlink failed\n");
    } else {
        printf("Cleanup successfully\n");
    }
    return 0;
}