#ifndef TRANSACTION_LOG_HPP
#define TRANSACTION_LOG_HPP

#include "transaction.hpp"
#include "mutex_guard.hpp"
#include <pthread.h>
#include <vector>
#include <atomic>
#include <ctime>

class TransactionLog {
private:
    std::vector<Transaction> transactions_;
    mutable pthread_mutex_t mutex_;
    std::atomic<int> nextId_;

public:
    TransactionLog() : nextId_(0) {
        if (pthread_mutex_init(&mutex_, nullptr) != 0) {
            throw std::runtime_error("Failed to initialize transaction log mutex");
        }
    }

    ~TransactionLog() {
        pthread_mutex_destroy(&mutex_);
    }

    TransactionLog(const TransactionLog&) = delete;
    TransactionLog& operator=(const TransactionLog&) = delete;

    void record(TransactionType type, int accountId, long long amountCents, long long balanceAfterCents,std::optional<int> relatedAccountId = std::nullopt) 
    {
        Transaction t;
        t.transactionId = nextId_++;
        t.type = type;
        t.accountId = accountId;
        t.relatedAccountId = relatedAccountId;
        t.amountCents = amountCents;
        t.balanceAfterCents = balanceAfterCents;
        t.timestamp = time(nullptr);

        MutexGuard guard(mutex_);
        transactions_.push_back(t);
    }

};

#endif