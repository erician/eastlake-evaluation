#ifndef UTIL_RWLOCK_H_
#define UTIL_RWLOCK_H_
#include <iostream>
// the return value of CAS is true or false
#define CAS(_p, _u, _v)  (__atomic_compare_exchange_n \
    (_p, _u, _v, false, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE))

const unsigned long kWriteStatus = ((unsigned long)1<<63);
const unsigned long kInitialValue = 0x0;

class RwLock {
public:
    RwLock(): lock_{kInitialValue} {};
    // this func won't block
    inline bool TryReadLock() {
        unsigned long cur_lock = lock_;
        if(cur_lock&kWriteStatus)
            return false;
        bool is_succeed = CAS(&lock_, &cur_lock, cur_lock+1);
        return is_succeed;
    }
    inline void ReadUnlock() {
        unsigned long cur_lock = lock_;
        while(!CAS(&lock_, &cur_lock, cur_lock-1)){
            cur_lock = lock_;
        }
    }
    // this func will block
    inline void WriteLock() {
        unsigned long cur_lock = kInitialValue;
        while(!CAS(&lock_, &cur_lock, kWriteStatus+1));
    }
    inline void WriteUnlock() {
        unsigned long cur_lock = lock_;
        CAS(&lock_, &cur_lock, kInitialValue);
    }
private:
    volatile unsigned long lock_;
};

#endif