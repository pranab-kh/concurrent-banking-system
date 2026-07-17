#ifndef MUTEX_GUARD_HPP
#define MUTEX_GUARD_HPP

#include <pthread.h>

class MutexGuard {
public:
    explicit MutexGuard(pthread_mutex_t& m) : mutex_(m) {
        pthread_mutex_lock(&mutex_);
    }

    ~MutexGuard() {
        pthread_mutex_unlock(&mutex_);
    }

    MutexGuard(const MutexGuard&) = delete;
    MutexGuard& operator=(const MutexGuard&) = delete;

private:
    pthread_mutex_t& mutex_;
};

#endif