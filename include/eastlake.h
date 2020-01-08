#ifndef EASTLAKE_H_
#define EASTLAKE_H_

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/syscall.h>   /* For SYS_xxx definitions */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>

#define PAGE_SIZE 4096

/*
 * struct po_stat
 */
struct po_stat {
	mode_t  st_mode;
	uid_t   st_uid;
	gid_t   st_gid;
	off_t   st_size;
};

/*
 * system calls in lullaby.
 * attention: we will not use these functions directly, 
 * instead we will use syscall by passing syscall number to it.
 */
static inline long po_creat(const char *poname, mode_t mode) {
	return syscall(400, poname, mode);
}

static inline long po_unlink(const char *poname) {
	return syscall(401, poname);
}

static inline long po_open(const char *poname, int flags, mode_t mode) {
	return syscall(402, poname, flags, mode);
}

static inline long po_close(unsigned int pod) {
	return syscall(403, pod);
}

static inline long po_mmap(unsigned long addr, unsigned long len, \
                 unsigned long prot, unsigned long flags, \
                 unsigned int pod, unsigned long pgoff) {
	return syscall(404, addr, len, prot, flags, pod, pgoff);
}

static inline long po_munmap(unsigned long addr, size_t len) {
	return syscall(405, addr, len);
}

static inline long po_extend(unsigned int pod, size_t len, \
	unsigned long prot, unsigned long flags) {
	return syscall(406, pod, len, prot, flags);
}

static inline long po_shrink(unsigned int pod, unsigned long addr, size_t len) {
	return syscall(407, pod, addr, len);
}

static inline long po_stat(const char *poname, struct po_stat *statbuf) {
	return syscall(408, poname, statbuf);
}

static inline long po_fstat(unsigned int pod, struct po_stat *statbuf) {
	return syscall(409, pod, statbuf);
}

/*
 * user library functions in lullaby
 */
#ifndef USE_SLAB

static void *po_malloc(int pod, size_t size) {
    long int retval = po_extend(pod, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE);
	if (retval == -1)
        return NULL;
    return (void*)retval;
}

static void po_free(int pod, void *ptr) {
    int retval;
    retval = po_shrink(pod, (unsigned long)ptr, 4096);
    if (retval == -1)
        printf("po_shring failed, errno: %d\n", errno);
}

#else
static void po_memory_alloc_init(struct slab_chain *const sch, const size_t itemsize);
extern struct slab_chain *sch;
extern int pod;
static void init_eastlake(const char *poname, size_t itemsize) {
	pod = po_creat(poname, 0);
	if (pod == -1) {
		printf("po_creat failed, errno: %d\n", errno);
		exit(-1);
	}
	sch = (struct slab_chain *)malloc(sizeof(struct slab_chain));
	sch->pod = pod;
	po_memory_alloc_init(sch, itemsize);
}

static void destroy_eastlake(const char *poname) {
	po_close(pod);
	po_unlink(poname);
}

static void po_memory_alloc_init(struct slab_chain *const sch, const size_t itemsize) {
	slab_init(sch, itemsize);
}

static void *po_malloc(struct slab_chain *const sch) {
    return slab_alloc(sch);
}

static void po_free(struct slab_chain *const sch, const void *const addr) {
    return slab_free(sch, addr);
}

#endif

#endif
