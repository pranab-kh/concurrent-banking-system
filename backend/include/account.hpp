#ifndef ACCOUNT_HPP
#define ACCOUNT_HPP

// #include "mutex_guard.hpp"

// #include <pthread.h>
// #include <string>
// #include <stdexcept>
#include "transaction.hpp"

class Account {
private:
    int account_id;
    std::string account_holder;
    int64_t actual_balance;
    int64_t available_balance;
    int64_t hold_amount;
    std::string account_status;
    std::string account_type;
    std::string account_created_at;
    std::string account_updated_at;
    std::unordered_map<int, Transaction> transactions;

public:
    // Default constructor
    Account() = default;

    // // Delete copy operations (non-copyable)
    Account(const Account&) = delete;
    Account& operator=(const Account&) = delete;

    // // Default move operations (move-only)
    Account(Account&&) = default;
    Account& operator=(Account&&) = default;

   
    Account(int account_id_,
            std::string account_holder_,
            int64_t actual_balance_,
            int64_t available_balance_,
            int64_t hold_amount_,
            std::string account_status_,
            std::string account_type_,
            std::string account_created_at_,
            std::string account_updated_at_,
            std::unordered_map<int, Transaction> transactions_)
        : account_id(std::move(account_id_)),
          account_holder(std::move(account_holder_)),
          actual_balance(std::move(actual_balance_)),
          available_balance(std::move(available_balance_)),
          hold_amount(std::move(hold_amount_)),
          account_status(std::move(account_status_)),
          account_type(std::move(account_type_)),
          account_created_at(std::move(account_created_at_)),
          account_updated_at(std::move(account_updated_at_)),
          transactions(std::move(transactions_)) {
    }
    // Getters
int get_account_id() const { return account_id; }
const std::string& get_account_holder() const { return account_holder; }
int64_t get_actual_balance() const { return actual_balance; }
int64_t get_available_balance() const { return available_balance; }
int64_t get_hold_amount() const { return hold_amount; }
const std::string& get_account_status() const { return account_status; }
const std::string& get_account_type() const { return account_type; }
const std::string& get_account_created_at() const { return account_created_at; }
const std::string& get_account_updated_at() const { return account_updated_at; }
const std::unordered_map<int, Transaction>& get_transactions() const { return transactions; }



};

// class Account {
// private:
//     int accountId_;
//     std::string ownerName_;
//     long long balanceCents_;
//     mutable pthread_mutex_t mutex_;

// public:
//     Account(int accountId, const std::string& ownerName, long long initialBalanceCents = 0)
//         : accountId_(accountId), ownerName_(ownerName), balanceCents_(initialBalanceCents)
//     {
//         if (initialBalanceCents < 0) {
//             throw std::invalid_argument("Initial balance cannot be negative");
//         }

//         if (pthread_mutex_init(&mutex_, nullptr) != 0) {
//             throw std::runtime_error("Failed to initialize account mutex");
//         }
//     }

//     ~Account() {
//         pthread_mutex_destroy(&mutex_);
//     }

//     Account(const Account&) = delete;
//     Account& operator=(const Account&) = delete;

//     bool deposit(long long amountCents) 
//     {
//         if (amountCents <= 0) {
//             throw std::invalid_argument("Deposit amount must be positive");
//         }

//         MutexGuard guard(mutex_);
//         balanceCents_ += amountCents;
//         return true;
//     }

//     bool withdraw(long long amountCents)
//     {
//         if (amountCents <= 0) {
//             throw std::invalid_argument("Withdrawal amount must be positive");
//         }

//         MutexGuard guard(mutex_);
//         if (amountCents > balanceCents_) {
//             return false;
//         }

//         balanceCents_ -= amountCents;
//         return true;
//     }

//     long long getBalance() const {
//         MutexGuard guard(mutex_);
//         return balanceCents_;
//     }

//     int getAccountId() const {
//         return accountId_;
//     }

//     std::string getOwnerName() const {
//         return ownerName_;
//     }

// };

#endif 