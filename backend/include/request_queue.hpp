#ifndef REQUEST_QUEUE_HPP
#define REQUEST_QUEUE_HPP

#include "transaction_request.hpp"
#include "mutex_guard.hpp"
#include <pthread.h>
#include <queue>
#include <stdexcept>
#include "mutex_guard.hpp"

#include<iostream> 


template <class T>
class circular_queue
{
private:
    int head = -1;
    int tail = -1;
    int max;
    T *arr;

public:
    circular_queue(int max = 10)
    {
        this->max = max;
        arr = new T[max];
        for (int i = 0; i < max; i++)
        {
            arr[i] = T();
        }
    }
    ~circular_queue()
    {
        delete[] arr;
    }

    bool push(T data)
    {

        if ((tail + 1) % max == head )
        {
            std::cout << "queue is full" << std::endl;
            return false;
        }
        if (head == -1)
        {
            head = 0;
        }
        tail = (tail + 1) % max;
        arr[tail] = data;
        return true;
    }
    bool pop()
    {
        if (head == -1)
        {
            std::cout << "Queue is empty" << std::endl;
            return false;
        }

        if (head == tail)
        {
            head = tail = -1;
        }
        else
        {
            head = (head + 1) % max;
        }
        return true;
    }
    void display()
    {

        if (head == -1)
        {
            std::cout << "Queue empty" << std::endl;
            return;
        }

        int i = head;
        while (true)
        {
            std::cout << arr[i] << "\t";
            if (i == tail)
                break; // stop after printing tail
            i = (i + 1) % max;
        }
        std::cout << std::endl;
    }
        bool isEmpty()
    {
        if(head == -1 && tail == -1)
        {
            return true;
        }
        return false;
    }

    bool isFull()
    {
        if((tail+1)%max==head)
        {
            return true;
        }
        return false;
    }

    T& front()
    {
        if (head == -1)
        {
            throw std::runtime_error("Queue is empty");
        }
        return arr[head];
    }

};

//templetaized the request queue used for both login and transaction
template <class T>
class RequestQueue {
private:
    circular_queue<T> queue_;
    pthread_mutex_t mutex_;
    pthread_cond_t notEmpty_;
    bool shuttingDown_ = false;

public:
    RequestQueue(int capacity = 1500) : queue_(capacity)
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

    bool push(T req) {
        MutexGuard guard(mutex_);
        if (shuttingDown_) return false;
        queue_.push(req);
        pthread_cond_signal(&notEmpty_);
        return true;
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

    
    bool pop(T& outReq) {

        MutexGuard guard(mutex_);
        while (queue_.isEmpty() && !shuttingDown_) {
            pthread_cond_wait(&notEmpty_, &mutex_);
        }
        if (queue_.isEmpty()) {
            // this is reachable if shuttingDown_ is true and nothing is left to drain
            return false;
        }
        outReq = queue_.front();
        queue_.pop();
        return true;
    }
};

#endif