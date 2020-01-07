#ifndef UTIL_PAIR_H_
#define UTIL_PAIR_H_

#include <cstdlib>
#include <iostream>

#define USE_EASTLAKE_JEMALLOC

#ifdef USE_AEP
#ifndef AEP_H_
#include "../aep.h"
#endif
#endif

#ifdef USE_LIBVMEMALLOC
#include <libvmmalloc.h>
#endif

typedef size_t Key_t;
typedef const char* Value_t;

const Key_t SENTINEL = -2; // 11111...110
const Key_t INVALID = -1; // 11111...111

const Value_t NONE = 0x0;

struct Pair {
  Key_t key;
  Value_t value;

  Pair(void)
  : key{INVALID} { }

  Pair(Key_t _key, Value_t _value)
  : key{_key}, value{_value} { }

  // Pair& operator=(const Pair& other) {
  //   key = other.key;
  //   value = other.value;
  // }

  void* operator new(size_t size) {
    void *ret;
#ifdef USE_AEP
    ret = vmem_malloc(size);
    if (ret == NULL){
        std::cout << "pair new[] vmem malloc" << std::endl;
      	exit(1);
    }
#else
#ifdef USE_LIBVMEMALLOC
     ret = memalign(kCacheLineSize,size);
     if(ret == NULL) {
       	std::cout << "pair new libvmemalloc" << std::endl;	
        exit(1);    
     }
#else
#ifdef USE_EASTLAKE_JEMALLOC
    posix_memalign(&ret, 64, size);
#else
    posix_memalign(&ret, 64, size);
#endif
#endif
#endif
    return ret;
  }

  void* operator new[](size_t size) {
    void *ret;
#ifdef USE_AEP
    ret = vmem_malloc(size);
    if (ret == NULL){
        std::cout << "pair new[] vmem malloc" << std::endl;
      	exit(1);
    }
#else
#ifdef USE_LIBVMEMALLOC
     ret = memalign(kCacheLineSize,size);
     if(ret == NULL) {
       	std::cout << "pair new[] libvmemalloc" << std::endl;	
        exit(1);    
     }
#else
#ifdef USE_EASTLAKE_JEMALLOC
    posix_memalign(&ret, 64, size);
#else
    posix_memalign(&ret, 64, size);
#endif
#endif
#endif
    return ret;
  }
};

#endif  // UTIL_PAIR_H_
