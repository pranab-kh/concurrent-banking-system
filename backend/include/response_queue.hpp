#ifndef RESPONSE_QUEUE_HPP
#define RESPONSE_QUEUE_HPP

#include "mutex_guard.hpp"
#include <pthread.h>
#include <queue>
#include <string>
#include <stdexcept>

struct TransactionResponse {
    int requestId;
    bool success;
    std::string message;
    long long newBalanceCents;
    std::string type;   // "DEPOSIT" / "WITHDRAW" / "TRANSFER"
    long long amount;
};

class ResponseQueue {
private:
    std::queue<TransactionResponse> queue_;
    pthread_mutex_t mutex_;
    pthread_cond_t notEmpty_;
    bool shuttingDown_ = false;

public:
    ResponseQueue() {
        if (pthread_mutex_init(&mutex_, nullptr) != 0) {
            throw std::runtime_error("Failed to initialize response queue mutex");
        }
        if (pthread_cond_init(&notEmpty_, nullptr) != 0) {
            pthread_mutex_destroy(&mutex_);
            throw std::runtime_error("Failed to initialize response queue condition variable");
        }
    }

    ~ResponseQueue() {
        pthread_mutex_destroy(&mutex_);
        pthread_cond_destroy(&notEmpty_);
    }

    ResponseQueue(const ResponseQueue&) = delete;
    ResponseQueue& operator=(const ResponseQueue&) = delete;

    void push(TransactionResponse resp) {
        MutexGuard guard(mutex_);
        if (shuttingDown_) return;
        queue_.push(resp);
        pthread_cond_signal(&notEmpty_);
    }

    bool pop(TransactionResponse& outResp) {
        MutexGuard guard(mutex_);
        while (queue_.empty() && !shuttingDown_) {
            pthread_cond_wait(&notEmpty_, &mutex_);
        }
        if (shuttingDown_ && queue_.empty()) {
            return false;
        }
        outResp = queue_.front();
        queue_.pop();
        return true;
    }

    void shutdown() {
        MutexGuard guard(mutex_);
        shuttingDown_ = true;
        pthread_cond_broadcast(&notEmpty_);
    }
};

#endif