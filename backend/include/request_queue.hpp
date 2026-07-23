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
        queue_.push(req);
        pthread_cond_signal(&notEmpty_);
    }

    TransactionRequest pop() {
        MutexGuard guard(mutex_);
        while (queue_.empty()) {
            pthread_cond_wait(&notEmpty_, &mutex_);
        }
        TransactionRequest req = queue_.front();
        queue_.pop();
        return req;
    }
};

#endif