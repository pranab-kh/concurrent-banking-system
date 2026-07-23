#ifndef REQUEST_QUEUE_HPP
#define REQUEST_QUEUE_HPP

#include "transaction_request.hpp"
#include "mutex_guard.hpp"
#include <pthread.h>
#include <queue>
#include <stdexcept>

class RequestQueue {
private:
    std::queue<TransactionRequest> queue_;
    pthread_mutex_t mutex_;
    pthread_cond_t notEmpty_;
    bool shuttingDown_ = false;

public:
    RequestQueue() 
    {
        if (pthread_mutex_init(&mutex_, nullptr) != 0) 
        {
            throw std::runtime_error("Failed to initialize queue mutex");
        }
        if (pthread_cond_init(&notEmpty_, nullptr) != 0) 
        {
            pthread_mutex_destroy(&mutex_);
            throw std::runtime_error("Failed to initialize condition variable");
        }
    }

    ~RequestQueue() 
    {
        pthread_mutex_destroy(&mutex_);
        pthread_cond_destroy(&notEmpty_);
    }

    RequestQueue(const RequestQueue&) = delete;
    RequestQueue& operator=(const RequestQueue&) = delete;

    void push(TransactionRequest req) {
        MutexGuard guard(mutex_);
        if (shuttingDown_) return;
        queue_.push(req);
        pthread_cond_signal(&notEmpty_);
    }


    void notifyAll() {
        MutexGuard guard(mutex_);
        pthread_cond_broadcast(&notEmpty_);
    }

    void shutdown() {
        MutexGuard guard(mutex_);
        shuttingDown_ = true;
        pthread_cond_broadcast(&notEmpty_);
    }

    
    bool pop(TransactionRequest& outReq) {
        MutexGuard guard(mutex_);
        while (queue_.empty() && !shuttingDown_) {
            pthread_cond_wait(&notEmpty_, &mutex_);
        }
        if (shuttingDown_ && queue_.empty()) {
            return false;
        }
        outReq = queue_.front();
        queue_.pop();
        return true;
    }
};

#endif