#include <iostream>
#include <iomanip>
#include <cmath>
#include <thread>
#include <bitset>
#include <cassert>
#include <unordered_map>
#include "util/persist.h"
#include "util/hash.h"
#include "CCEH.h"

extern size_t perfCounter;
unsigned long clflush_seg_cnt_ = 0;
unsigned long clflush_dir_cnt_ = 0;

int Segment::Insert(Key_t& key, Value_t value, size_t loc, size_t key_hash) {
#ifdef INPLACE
  //if (sema == -1) return 2;
  //if ((key_hash >> (8*sizeof(key_hash)-local_depth)) != pattern) return 2;
  //auto lock = sema;
  int ret = 1;
  // while (!CAS(&sema, &lock, lock+1)) {
  //   lock = sema;
  // }
  //Key_t LOCK = INVALID;
  for (unsigned i = 0; i < kNumPairPerCacheLine * kNumCacheLine; ++i) {
    auto slot = (loc + i) % kNumSlot;
   //  Key_t LOCK1 = _[slot].key;
    if ( _[slot].key==INVALID \
        || ((h(&_[slot].key,sizeof(Key_t)) >> (8*sizeof(key_hash)-local_depth)) != pattern)) {
       _[slot].value = value;
       mfence();
      _[slot].key = key;
      clflush((char*)&_[slot], sizeof(Pair));
      ret = 0;
      break;
    }
    
  }
  // lock = sema;
  // while (!CAS(&sema, &lock, lock-1)) {
  //   lock = sema;
  // }
  return ret;
#else
  // if (sema == -1) return 2;
  // if ((key_hash >> (8*sizeof(key_hash)-local_depth)) != pattern) return 2;
  // auto lock = sema;
  int ret = 1;
  // while (!CAS(&sema, &lock, lock+1)) {
  //   lock = sema;
  // }
 // Key_t LOCK = INVALID;
  for (unsigned i = 0; i < kNumPairPerCacheLine * kNumCacheLine; ++i) {
    auto slot = (loc + i) % kNumSlot;
    if (_[slot].key==INVALID) {
      _[slot].value = value;
      mfence();
      _[slot].key = key;
      ret = 0;
      break;
    } //else {
      //LOCK = INVALID;
    //}
  }
  // lock = sema;
  // while (!CAS(&sema, &lock, lock-1)) {
  //   lock = sema;
  // }
  return ret;
#endif
}

void Segment::Insert4split(Key_t& key, Value_t value, size_t loc) {
  for (unsigned i = 0; i < kNumPairPerCacheLine * kNumCacheLine; ++i) {
    auto slot = (loc+i) % kNumSlot;
    if (_[slot].key == INVALID) {
      _[slot].key = key;
      _[slot].value = value;
      return;
    }
  }
}

Segment** Segment::Split(void) {
  using namespace std;
  int64_t lock = 0;
  //if (!CAS(&sema, &lock, -1)) return nullptr;

#ifdef INPLACE
  Segment** split = new Segment*[2];
  split[0] = this;
  split[1] = new Segment(local_depth+1);

  for (unsigned i = 0; i < kNumSlot; ++i) {
    auto key_hash = h(&_[i].key, sizeof(Key_t));
    if (key_hash & ((size_t) 1 << ((sizeof(Key_t)*8 - local_depth - 1)))) {
      split[1]->Insert4split
        (_[i].key, _[i].value, (key_hash & kMask)*kNumPairPerCacheLine);
    }
  }
  clflush_seg_cnt_++;
  clflush((char*)split[1], sizeof(Segment));
  local_depth = local_depth + 1;
  clflush((char*)&local_depth, sizeof(size_t));

  return split;
#else
  Segment** split = new Segment*[2];
  split[0] = new Segment(local_depth+1);
  split[1] = new Segment(local_depth+1);

  for (unsigned i = 0; i < kNumSlot; ++i) {
    auto key_hash = h(&_[i].key, sizeof(Key_t));
    if (key_hash & ((size_t) 1 << ((sizeof(Key_t)*8 - local_depth - 1)))) {
      split[1]->Insert4split
        (_[i].key, _[i].value, (key_hash & kMask)*kNumPairPerCacheLine);
    } else {
      split[0]->Insert4split
        (_[i].key, _[i].value, (key_hash & kMask)*kNumPairPerCacheLine);
    }
  }

  clflush((char*)split[0], sizeof(Segment));
  clflush((char*)split[1], sizeof(Segment));

  return split;
#endif
}


CCEH::CCEH(void)
: dir{1}, global_depth{0}
{
  for (unsigned i = 0; i < dir.capacity; ++i) {
    dir._[i] = new Segment(global_depth);
    dir._[i]->pattern = i;
  }
  dir_doubling_cnt_ = 0;
  breakdown_maintenance_dir_ = 0;
  dir_doubling_time_ =0;
  linear_probing_ =0;
  split_num_=0;
}

CCEH::CCEH(size_t initCap)
: dir{initCap}, global_depth{static_cast<size_t>(log2(initCap))}
{
  for (unsigned i = 0; i < dir.capacity; ++i) {
    dir._[i] = new Segment(global_depth);
    dir._[i]->pattern = i;
  }
  dir_doubling_cnt_ = 0;
  breakdown_maintenance_dir_ = 0;
  dir_doubling_time_ =0;
  split_num_=0;
}

CCEH::~CCEH(void)
{ }

void Directory::LSBUpdate(int local_depth, int global_depth, int dir_cap, int x, Segment** s) {
  int depth_diff = global_depth - local_depth;
  if (depth_diff == 0) {
    if ((x % dir_cap) >= dir_cap/2) {
      _[x-dir_cap/2] = s[0];
      clflush((char*)&_[x-dir_cap/2], sizeof(Segment*));
      _[x] = s[1];
      clflush((char*)&_[x], sizeof(Segment*));
    } else {
      _[x] = s[0];
      clflush((char*)&_[x], sizeof(Segment*));
      _[x+dir_cap/2] = s[1];
      clflush((char*)&_[x+dir_cap/2], sizeof(Segment*));
    }
  } else {
    if ((x%dir_cap) >= dir_cap/2) {
      LSBUpdate(local_depth+1, global_depth, dir_cap/2, x-dir_cap/2, s);
      LSBUpdate(local_depth+1, global_depth, dir_cap/2, x, s);
    } else {
      LSBUpdate(local_depth+1, global_depth, dir_cap/2, x, s);
      LSBUpdate(local_depth+1, global_depth, dir_cap/2, x+dir_cap/2, s);
    }
  }
  return;
}

void CCEH::Insert(Key_t& key, Value_t value) {
STARTOVER:
  auto key_hash = h(&key, sizeof(key));
  auto y = (key_hash & kMask) * kNumPairPerCacheLine;

RETRY:
  auto x = (key_hash >> (8*sizeof(key_hash)-global_depth));
  auto target = dir._[x];
  auto ret = target->Insert(key, value, y, key_hash);

  if (ret == 1) {
    // print current load factor
   // std::cout << Utilization() << std::endl;
    timer.Start();
    Segment** s = target->Split();
    timer.Stop();
    split_num_++;
    breakdown_seg_split_ += timer.GetSeconds();
    if (s == nullptr) {
      // another thread is doing split
      goto RETRY;
    }

    s[0]->pattern = (key_hash >> (8*sizeof(key_hash)-s[0]->local_depth+1)) << 1;
    s[1]->pattern = ((key_hash >> (8*sizeof(key_hash)-s[1]->local_depth+1)) << 1) + 1;

    
    // Directory management
    // while (!dir.Acquire()) {
    //   asm("nop");
    // }
    { // CRITICAL SECTION - directory update
      x = (key_hash >> (8*sizeof(key_hash)-global_depth));
#ifdef INPLACE
      if (dir._[x]->local_depth-1 < global_depth) {  // normal split
#else
      if (dir._[x]->local_depth < global_depth) {  // normal split
#endif
        unsigned depth_diff = global_depth - s[0]->local_depth;
        if (depth_diff == 0) {
          if (x%2 == 0) {
            dir._[x+1] = s[1];
#ifdef INPLACE
            clflush((char*) &dir._[x+1], 8);
            clflush_dir_cnt_++;
#else
            mfence();
            dir._[x] = s[0];
            clflush((char*) &dir._[x], 16);
#endif
          } else {
            dir._[x] = s[1];
#ifdef INPLACE
            clflush((char*) &dir._[x], 8);
            clflush_dir_cnt_++;
#else
            mfence();
            dir._[x-1] = s[0];
            clflush((char*) &dir._[x-1], 16);
#endif
          }
        } else {
          int chunk_size = pow(2, global_depth - (s[0]->local_depth - 1));
          x = x - (x % chunk_size);
          for (unsigned i = 0; i < chunk_size/2; ++i) {
            dir._[x+chunk_size/2+i] = s[1];
          }
          clflush((char*)&dir._[x+chunk_size/2], sizeof(void*)*chunk_size/2);
          clflush_dir_cnt_ += (64+(sizeof(void*)*chunk_size/2))/64;
#ifndef INPLACE
          for (unsigned i = 0; i < chunk_size/2; ++i) {
            dir._[x+i] = s[0];
          }
          clflush((char*)&dir._[x], sizeof(void*)*chunk_size/2);
#endif
        }
      } else {  // directory doubling
       // timer.Start();
        auto d = dir._;
        Segment **_dir;
#ifdef USE_AEP
    _dir = (Segment **)vmem_malloc(dir.capacity*2*8);
    if (_dir == NULL){
        std::cout << "aep _dir" << std::endl;
        exit(1);
    }
#else
#ifdef USE_LIBVMEMALLOC
     _dir = (Segment **)memalign(kCacheLineSize, dir.capacity*2*8);
     if(ret == NULL) {
        std::cout << "libvmemalloc _dir" << std::endl;
        exit(1);
     }
#else
#ifdef USE_EASTLAKE_JEMALLOC
    posix_memalign((void **)(&_dir), 64, dir.capacity*2*8);
#else
    posix_memalign(&_dir, 64, dir.capacity*2*8);
#endif
#endif
#endif

        for (unsigned i = 0; i < dir.capacity; ++i) {
          if (i == x) {
            _dir[2*i] = s[0];
            _dir[2*i+1] = s[1];
          } else {
            _dir[2*i] = d[i];
            _dir[2*i+1] = d[i];
          }
        }
        clflush((char*)&dir._[0], sizeof(Segment*)*dir.capacity);
        clflush_dir_cnt_ += (64+sizeof(Segment*)*dir.capacity/64);
        dir._ = _dir;
        clflush((char*)&dir._, sizeof(void*));
        clflush_dir_cnt_++;
        dir.capacity *= 2;
        clflush((char*)&dir.capacity, sizeof(size_t));
        clflush_dir_cnt_++;
        global_depth += 1;
        clflush((char*)&global_depth, sizeof(global_depth));
        clflush_dir_cnt_++;
        //delete d;
        // TODO: requiered to do this atomically
      
       // timer.Stop();
       // dir_doubling_cnt_++;
       //  dir_doubling_time_ = timer.GetSeconds();
        // std::cout << "dir doubling time:" ;
       //  std::cout<<std::setiosflags(std::ios::fixed);
        //  std::cout<<std::setprecision(8)<<dir_doubling_time_ <<std::endl;
      }
#ifdef INPLACE
     // s[0]->sema = 0;
#endif
    }  // End of critical section
    // while (!dir.Release()) {
    //   asm("nop");
    // }
   
    //breakdown_maintenance_dir_ += timer.GetSeconds();
   // std::cout << "dir doubling time:"<<breakdown_maintenance_dir_ <<std::endl;
    goto RETRY;
  } else if (ret == 2) {
    // Insert(key, value);
    goto STARTOVER;
  } else {
    clflush((char*)&dir._[x]->_[y], 64);
  }
}

// This function does not allow resizing
bool CCEH::InsertOnly(Key_t& key, Value_t value) {
  auto key_hash = h(&key, sizeof(key));
  auto x = (key_hash >> (8*sizeof(key_hash)-global_depth));
  auto y = (key_hash & kMask) * kNumPairPerCacheLine;

  auto ret = dir._[x]->Insert(key, value, y, key_hash);
  if (ret == 0) {
    clflush((char*)&dir._[x]->_[y], 64);
    return true;
  }

  return false;
}

// TODO
bool CCEH::Delete(Key_t& key) {
  return false;
}

Value_t CCEH::Get(Key_t& key) {
  auto key_hash = h(&key, sizeof(key));
  auto x = (key_hash >> (8*sizeof(key_hash)-global_depth));
  auto y = (key_hash & kMask) * kNumPairPerCacheLine;

  auto dir_ = dir._[x];

#ifdef INPLACE
  // auto sema = dir._[x]->sema;
  // while (!CAS(&dir._[x]->sema, &sema, sema+1)) {
  //   sema = dir._[x]->sema;
  // }
#endif

  for (unsigned i = 0; i < kNumPairPerCacheLine * kNumCacheLine; ++i) {
    auto slot = (y+i) % Segment::kNumSlot;
     if(i%4 ==0)
       linear_probing_++;
    if (dir_->_[slot].key == key) {
#ifdef INPLACE
      // sema = dir._[x]->sema;
      // while (!CAS(&dir._[x]->sema, &sema, sema-1)) {
      //   sema = dir._[x]->sema;
      // }
#endif
      return dir_->_[slot].value;
    }
  }

#ifdef INPLACE
  // sema = dir._[x]->sema;
  // while (!CAS(&dir._[x]->sema, &sema, sema-1)) {
  //   sema = dir._[x]->sema;
  // }
#endif
  return NONE;
}

double CCEH::Utilization(void) {
  size_t sum = 0;
  Key_t tmp_key;
  unsigned long key_hash ;
  unsigned long tmp_pattern;
  
  std::unordered_map<Segment*, bool> set;
  for (size_t i = 0; i < dir.capacity; ++i) {
    set[dir._[i]] = true;
  }
  for (auto& elem: set) {
    for (unsigned i = 0; i < Segment::kNumSlot; ++i) {
      key_hash = h(&elem.first->_[i].key, sizeof(Key_t));
      tmp_pattern = key_hash >> (8*sizeof(key_hash)- elem.first->local_depth);
      if (elem.first->_[i].key != INVALID && tmp_pattern == elem.first->pattern ) 
          sum++;
    }
  }
  return ((double)sum)/((double)set.size()*Segment::kNumSlot)*100.0;
}

size_t CCEH::Capacity(void) {
  std::unordered_map<Segment*, bool> set;
  for (size_t i = 0; i < dir.capacity; ++i) {
    set[dir._[i]] = true;
  }
  return set.size() * Segment::kNumSlot;
}

size_t Segment::numElem(void) {
  size_t sum = 0;
  for (unsigned i = 0; i < kNumSlot; ++i) {
    if (_[i].key != INVALID) {
      sum++;
    }
  }
  return sum;
}

bool CCEH::Recovery(void) {
  bool recovered = false;
  size_t i = 0;
  while (i < dir.capacity) {
    size_t depth_cur = dir._[i]->local_depth;
    size_t stride = pow(2, global_depth - depth_cur);
    size_t buddy = i + stride;
    if (buddy == dir.capacity) break;
    for (int j = buddy - 1; i < j; j--) {
      if (dir._[j]->local_depth != depth_cur) {
        dir._[j] = dir._[i];
      }
    }
    i = i+stride;
  }
  if (recovered) {
    clflush((char*)&dir._[0], sizeof(void*)*dir.capacity);
  }
  return recovered;
}

// for debugging
Value_t CCEH::FindAnyway(Key_t& key) {
  using namespace std;
  for (size_t i = 0; i < dir.capacity; ++i) {
     for (size_t j = 0; j < Segment::kNumSlot; ++j) {
       if (dir._[i]->_[j].key == key) {
         auto key_hash = h(&key, sizeof(key));
         auto x = (key_hash >> (8*sizeof(key_hash)-global_depth));
         auto y = (key_hash & kMask) * kNumPairPerCacheLine;
         cout << bitset<32>(i) << endl << bitset<32>((x>>1)) << endl << bitset<32>(x) << endl;
         return dir._[i]->_[j].value;
       }
     }
  }
  return NONE;
}

void Directory::SanityCheck(void* addr) {
  using namespace std;
  for (unsigned i = 0; i < capacity; ++i) {
    if (_[i] == addr) {
      //cout << i << " " << _[i]->sema << endl;
      exit(1);
    }
  }
}
