#ifndef AEP_H_
#include "aep.h"
#endif

VMEM *vmp;

void init_vmp() {
	unsigned long pool_size =  40000000ull;
	if ((vmp = vmem_create("/home/eric/mypmemfs/pmdir", \
		pool_size)) == NULL) {
		perror("vmem_create");
		exit(1);
	}
}

void delete_vmp() {
	vmem_delete(vmp);
}

void* vmem_malloc(unsigned long size) {
	//printf("%p\n", vmp);
	return (void*)vmem_malloc(vmp, size);
}

void vmem_free(void *ptr) {
	vmem_free(vmp, ptr);
}



