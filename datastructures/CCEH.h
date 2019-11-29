#ifndef CCEH_H_
#define CCEH_H_

#include <cstring>
#include <cmath>
#include <vector>
#include <iostream>

#ifndef UTIL_PAIR_H_
#include "util/pair.h"
#endif

#ifndef UTIL_PERSIST_H_
#include "util/persist.h"
#endif

#ifndef HASH_INTERFACE_H_
#include "hash.h"
#endif

#define INPLACE
/* #define USE_AEP */
#define USE_EASTLAKE
#define USE_SLAB

#ifdef USE_EASTLAKE
#ifndef EASTLAKE_H_
#include "eastlake.h"
#endif
#endif

#ifdef USE_AEP
#ifndef AEP_H_
#include "aep.h"
#endif
#endif

#ifdef USE_LIBVMEMALLOC
#include <libvmmalloc.h>
#endif

//statistic information
extern unsigned long clflush_seg_cnt_;
extern unsigned long clflush_dir_cnt_;

constexpr size_t kSegmentBits = 8;
constexpr size_t kMask = (1 << kSegmentBits)-1;
constexpr size_t kShift = kSegmentBits;
constexpr size_t kSegmentSize = (1 << kSegmentBits) * 16 * 4;
constexpr size_t kNumPairPerCacheLine = 4;
constexpr size_t kNumCacheLine = 4; //kCacheLineSize/sizeof(Pair);

struct Segment {
  static const size_t kNumSlot = kSegmentSize/sizeof(Pair);

  Segment(void)
  : local_depth{0}
  { }

  Segment(size_t depth)
  :local_depth{depth}
  { }

  ~Segment(void) {
  }

  void* operator new(size_t size) {
    void* ret;
#ifdef USE_AEP
    ret = vmem_malloc(size);
    if (ret == NULL){
        std::cout << "segment new vmem malloc" << std::endl;
      	exit(1);
    }
#else
#ifdef USE_LIBVMEMALLOC
     ret = memalign(kCacheLineSize,size);
     if(ret == NULL) {
       	std::cout << "segment new vmem malloc" << std::endl;	
        exit(1);    
     }
#else
#ifdef USE_EASTLAKE
    ret = po_malloc(sch);
#else
    posix_memalign(&ret, 64, size);
#endif
#endif
#endif
    return ret;
  }

  void* operator new[](size_t size) {
    void* ret;
#ifdef USE_AEP
    ret = vmem_malloc(size);
    if (ret == NULL){
        std::cout << "segment new[] vmem malloc" << std::endl;
      	exit(1);
    }
#else
#ifdef USE_LIBVMEMALLOC
     ret = memalign(kCacheLineSize,size);
     if(ret == NULL) {
       	std::cout << "segment new[] libvmemalloc" << std::endl;	
        exit(1);    
     }
#else
#ifdef USE_EASTLAKE
    ret = po_malloc(sch);
#else
    posix_memalign(&ret, 64, size);
#endif
#endif
#endif
    return ret;
  }

  int Insert(Key_t&, Value_t, size_t, size_t);
  void Insert4split(Key_t&, Value_t, size_t);
  bool Put(Key_t&, Value_t, size_t);
  Segment** Split(void);

  Pair _[kNumSlot];
  size_t local_depth;
  //int64_t sema = 0;
  size_t pattern = 0;
  size_t numElem(void); 
};

struct Directory {
  static const size_t kDefaultDirectorySize = 1024;
  Segment** _;
  size_t capacity;
 // bool lock;
  //int sema = 0 ;

  Directory(void) {
    capacity = kDefaultDirectorySize;
#ifdef USE_AEP
   if ((_ = (Segment **)vmem_malloc(capacity*8)) == NULL){
     std::cout << "dir vmem malloc" << std::endl;
	   exit(1);
    }
#else
#ifdef USE_LIBVMEMALLOC
   if((_ = (Segment **)memalign(kCacheLineSize,capacity*8)) == NULL) {
     std::cout << "dir libvmemalloc error" << std::endl;
	   exit(1);
    }
#else
#ifdef USE_EASTLAKE
    _ = (Segment**)po_extend(pod, PAGE_SIZE*((capacity*8+PAGE_SIZE-1)/PAGE_SIZE), \
      PROT_READ|PROT_WRITE, MAP_PRIVATE);
#else
    _ = new Segment*[capacity];
#endif
#endif
#endif
  }

  Directory(size_t size) {
    capacity = size;
 #ifdef USE_AEP
   if ((_ = (Segment **)vmem_malloc(capacity*8)) == NULL){
     std::cout << "dir vmem malloc" << std::endl;
	   exit(1);
    }
#else
#ifdef USE_LIBVMEMALLOC
   if((_ = (Segment **)memalign(kCacheLineSize,capacity*8)) == NULL) {
     std::cout << "dir libvmemalloc error" << std::endl;
	   exit(1);
    }
#else
#ifdef USE_EASTLAKE
    _ = (Segment**)po_extend(pod, PAGE_SIZE*((capacity*8+PAGE_SIZE-1)/PAGE_SIZE), \
      PROT_READ|PROT_WRITE, MAP_PRIVATE);
#else
    _ = new Segment*[capacity];
#endif
#endif
#endif
  }

  ~Directory(void) {
    delete [] _;
  }

  // bool Acquire(void) {
  // bool unlocked = false;
  //   return CAS(&lock, &unlocked, true);
  // }

  // bool Release(void) {
  //   bool locked = true;
  //   return CAS(&lock, &locked, false);
  // }
  
  void SanityCheck(void*);
  void LSBUpdate(int, int, int, int, Segment**);
};

class CCEH : public Hash {
  public:
    int dir_doubling_cnt_;
    double breakdown_maintenance_dir_;
    double dir_doubling_time_;
    double linear_probing_;
    double split_num_;
    CCEH(void);
    CCEH(size_t);
    ~CCEH(void);
    void Insert(Key_t&, Value_t);
    bool InsertOnly(Key_t&, Value_t);
    bool Delete(Key_t&);
    Value_t Get(Key_t&);
    Value_t FindAnyway(Key_t&);
    double Utilization(void);
    size_t Capacity(void);
    bool Recovery(void);
    unsigned long StatisticGetDirectoryOccupiedSpace() {
      return dir.capacity * 8;
    }

    void* operator new(size_t size) {
      void *ret;
#ifdef USE_AEP
    ret = vmem_malloc(size);
    if (ret == NULL){
        std::cout << "segment new[] vmem malloc" << std::endl;
      	exit(1);
    }
#else
#ifdef USE_LIBVMEMALLOC
     ret = memalign(kCacheLineSize,size);
     if(ret == NULL) {
       	std::cout << "segment new[] vmem malloc" << std::endl;	
        exit(1);    
     }
#else
    posix_memalign(&ret, 64, size);
#endif
#endif
      return ret;
    }

  private:
    size_t global_depth;
    Directory dir;
};

#endif  // EXTENDIBLE_PTR_H_
