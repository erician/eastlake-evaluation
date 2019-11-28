#ifndef AEP_H_
#define AEP_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libvmem.h>

extern VMEM *vmp;

extern void init_vmp();

extern void delete_vmp();

extern void* vmem_malloc(unsigned long size);

extern void vmem_free(void *ptr);

#endif


