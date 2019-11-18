/*
 * this code refesrs NOVA's microbenchmarks.
 */

#ifndef EASTLAKE_H_
#include "eastlake.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <malloc.h>
#include <errno.h>
#include <string.h>

#define PONAMES_FILE            "ponames.txt"
#define OBJECT_OR_FILE_NUMBER   1000
#define PAGE_SIZE               (4*1024)
#define APPEND_SIZE             (PAGE_SIZE*16)

//#define TEST_FS

#ifdef TEST_FS
void my_create(const char *pathname) {
    int fd = creat(pathname, 0777);
    if (fd != -1)
	close(fd);
    else {
	printf("creat failed, pathname: %s, errno: %d", pathname, errno);
	exit(-1);
    }
}
void my_delete(const char *pathname) {
    int tmp = unlink(pathname);
    if (tmp == -1) {
	printf("delete failed, pathname: %s, errno: %d", pathname, errno);
	exit(-1);
    }
}
void my_open_and_close(const char *pathname) {
    int fd = open(pathname, O_RDWR);
    if (fd != -1)
        close(fd);
    else {
        printf("open failed, pathname: %s, errno: %d", pathname, errno);
	exit(-1);
    }
}
void my_append(const char *pathname, char *buf) {
    int fd = open(pathname, O_RDWR);
    if (fd == -1) {
        printf("append failed, pathname: %s, errno: %d", pathname, errno);
	exit(-1);
    }
    for(int i=0; i<APPEND_SIZE/PAGE_SIZE; i++)
        write(fd, buf, PAGE_SIZE);
    close(fd);
}
#else
void my_create(const char *poname) {
    int pod = po_creat(poname, 0777);
    if (pod != -1)
	po_close(pod);
    else {
	    printf("creat failed, poname: %s, errno: %d", poname, errno);
	    exit(-1);
    }
}
void my_delete(const char *poname) {
    int tmp = po_unlink(poname);
    if (tmp == -1) {
	    printf("delete failed, poname: %s, errno: %d", poname, errno);
	    exit(-1);
    }
}
void my_open_and_close(const char *poname) {
    int pod = po_open(poname, O_RDWR, 0777);
    if (pod != -1)
        po_close(pod);
    else {
        printf("open failed, pathname: %s, errno: %d", poname, errno);
	    exit(-1);
    }
}
void my_append(const char *poname, char *buf) {
    int pod = po_open(poname, O_RDWR, 0777);
    if (pod == -1) {
        printf("append failed, pathname: %s, errno: %d", poname, errno);
	    exit(-1);
    }
    char *c = (char *)po_extend(pod, APPEND_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE);
    for(int i=0; i<APPEND_SIZE/PAGE_SIZE; i++)
        memcpy(c, buf, PAGE_SIZE);
    po_close(pod);
}
#endif

double my_second() {
    struct timeval tp;
    int i;
    i = gettimeofday(&tp, NULL);
    return ( (double)tp.tv_sec + (double)tp.tv_usec*1.e-6);
}

void read_file(const char *ponames_file, char ponames[][32]) {
    FILE *fp;
    if((fp = fopen(ponames_file, "r")) == NULL) {
        printf("open ponames files failed");
        exit(-1);
    }
    int cnt = 0;
    while (!feof(fp)) {
        fgets(ponames[cnt], 31, fp);
        cnt++;
        if (cnt >= OBJECT_OR_FILE_NUMBER)
            break;
    }
}

int main() {
    char ponames[OBJECT_OR_FILE_NUMBER][32];
    read_file(PONAMES_FILE, ponames);

    double start, end;
    // create object or files
    char *poname;
    int cnt = 0;
    start = my_second();
    for(int i=0; i<OBJECT_OR_FILE_NUMBER; i++)
        my_create(ponames[i]);
    end = my_second();
#ifdef TEST_FS
    printf("creating files finished\n");
#else
    printf("create objects finished\n");
#endif
    printf("creat:\n");
    printf("number: %d, time: %lf, throught: %lf\n", \
        OBJECT_OR_FILE_NUMBER, end-start, OBJECT_OR_FILE_NUMBER/(end-start));

    // append data to object or file
    char *buf = (char*)malloc(PAGE_SIZE);
    cnt = 0;
    start = my_second();
    for(int i=0; i<OBJECT_OR_FILE_NUMBER; i++)
        my_append(ponames[i], buf);
    end = my_second();
    printf("append:\n");
    printf("number: %d, time: %lf, throught: %lf\n", \
        OBJECT_OR_FILE_NUMBER, end-start, OBJECT_OR_FILE_NUMBER/(end-start));
    
    // open and close objects or files
    cnt = 0;
    start = my_second();
    for(int i=0; i<OBJECT_OR_FILE_NUMBER; i++)
        my_open_and_close(ponames[i]);
    end = my_second();
    printf("open:\n");
    printf("number: %d, time: %lf, throught: %lf\n", \
        OBJECT_OR_FILE_NUMBER, end-start, OBJECT_OR_FILE_NUMBER/(end-start));

    // delete objects or files
    cnt = 0;
    start = my_second();
    for(int i=0; i<OBJECT_OR_FILE_NUMBER; i++)
        my_delete(ponames[i]);
    end = my_second();
    printf("delete:\n");
    printf("number: %d, time: %lf, throught: %lf\n", \
        OBJECT_OR_FILE_NUMBER, end-start, OBJECT_OR_FILE_NUMBER/(end-start));

    return 0;
}
