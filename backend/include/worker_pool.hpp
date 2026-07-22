#ifndef WORKER_POOL_HPP
#define WORKER_POOL_HPP

#include "request_queue.hpp"
#include "bank.hpp"
#include <pthread.h>
#include <vector>

class WorkerPool {
private:
    RequestQueue& queue_;
    Bank& bank_;
    std::vector<pthread_t> workers_;
    int numWorkers_;
    std::atomic<bool> shuttingDown_;


    static void* workerLoop(void* arg) {
        WorkerPool* pool = static_cast<WorkerPool*>(arg);
        pool->run();
        return nullptr;
    }

    void run() {
        while (!shuttingDown_) {
            TransactionRequest req = queue_.pop();
            if (shuttingDown_) break;
            bank_.process(req);
        }
    }

public:
    WorkerPool(Bank& bank, RequestQueue& queue, int numWorkers = 4)
        : bank_(bank), queue_(queue), numWorkers_(numWorkers), shuttingDown_(false)
    {
        for (int i = 0; i < numWorkers_; i++) {
            pthread_t thread;
            if (pthread_create(&thread, nullptr, workerLoop, this) != 0) {
                throw std::runtime_error("Failed to create worker thread");
            }
            workers_.push_back(thread);
        }
    }
};

#endif 