#ifndef REQUEST_QUEUE_HPP
#define REQUEST_QUEUE_HPP

#include "transaction_request.hpp"
#include <pthread.h>
#include <queue>
#include <stdexcept>
#include "mutex_guard.hpp"

//templetaized the request queue used for both login and transaction
template <class T>
class RequestQueue {
private:
    std::queue<T> queue_;
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

    bool enqueue(T data)
    {
        //add limit of queue
        MutexGuard mutex;
        queue_.push(data);
        return true;
    }

    T dequeue()
    {
        MutexGuard mutex;
        T res;
        res = queue_.front();
        queue_.pop();
        return res;
    }

    
};
#endif