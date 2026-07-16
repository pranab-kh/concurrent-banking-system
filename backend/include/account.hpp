#ifndef ACCOUNT_HPP
#define ACCOUNT_HPP

#include <pthread.h>
#include <string>
#include <stdexcept>

class Account {
private:
    int accountId_;
    std::string ownerName_;
    long long balanceCents_;
    mutable pthread_mutex_t mutex_;

public:
    Account(int accountId, const std::string& ownerName, long long initialBalanceCents = 0)
        : accountId_(accountId), ownerName_(ownerName), balanceCents_(initialBalanceCents)
    {
        if (initialBalanceCents < 0) {
            throw std::invalid_argument("Initial balance cannot be negative");
        }

        if (pthread_mutex_init(&mutex_, nullptr) != 0) {
            throw std::runtime_error("Failed to initialize account mutex");
        }
    }

    ~Account() {
        pthread_mutex_destroy(&mutex_);
    }

    Account(const Account&) = delete;
    Account& operator=(const Account&) = delete;

};

#endif 