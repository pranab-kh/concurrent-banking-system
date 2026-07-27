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
    std::unordered_map<int ,Transaction> transactions_;
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

    //requires fixing -- ##
    void record(TransactionType type, int accountId, long long amountCents, long long balanceAfterCents,std::optional<int> relatedAccountId = std::nullopt) 
    {
        //sort the parameters --#
        Transaction t(transaction_id, from_account, to_account, transaction_amount, receiver_name, receiver_mobile, remarks, transaction_status, transaction_at, transaction_type);

        MutexGuard guard(mutex_);
       transactions[transaction_id] = std::move(t);
    }

    std::vector<Transaction> getHistoryForAccount(int accountId) const
    {
        MutexGuard guard(mutex_);

        std::vector<Transaction> result;
        for (const auto& t : transactions_) {
            if (t.accountId == accountId) {
                result.push_back(t);
            }
        }
        return result;
    }

    std::vector<Transaction> getAllTransactions() const
    {
        MutexGuard guard(mutex_);
        return transactions_;
    }

};

#endif