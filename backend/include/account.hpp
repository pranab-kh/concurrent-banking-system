#ifndef ACCOUNT_HPP
#define ACCOUNT_HPP

#include "mutex_guard.hpp"
#include <pthread.h>
#include <string>
#include <stdexcept>
#include "transaction.hpp"
#include "hashtable.hpp"  

class Account
{
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
    HashTable<int, std::shared_ptr<Transaction>> transactions;   
    mutable pthread_mutex_t mutex_;

public:
    // Default constructor
    Account() = default;

    // // Delete copy operations (non-copyable)
    Account(const Account &) = delete;
    Account &operator=(const Account &) = delete;

    // // Default move operations (move-only)
    Account(Account &&) = default;
    Account &operator=(Account &&) = default;

    Account(int account_id_,
            std::string account_holder_,
            int64_t actual_balance_,
            int64_t available_balance_,
            int64_t hold_amount_,
            std::string account_status_,
            std::string account_type_,
            std::string account_created_at_,
            std::string account_updated_at_,
            HashTable<int, std::shared_ptr<Transaction>> transactions_) 
        : account_id(std::move(account_id_)),
          account_holder(std::move(account_holder_)),
          actual_balance(std::move(actual_balance_)),
          available_balance(std::move(available_balance_)),
          hold_amount(std::move(hold_amount_)),
          account_status(std::move(account_status_)),
          account_type(std::move(account_type_)),
          account_created_at(std::move(account_created_at_)),
          account_updated_at(std::move(account_updated_at_)),
          transactions(std::move(transactions_)) // this now uses hashtable's move constructor
    {
    }

    // Getters
    int get_account_id() const { return account_id; }
    const std::string &get_account_holder() const { return account_holder; }
    int64_t get_actual_balance() const 
    {
        MutexGuard guard(mutex_);
        return actual_balance;
    }
    
    int64_t get_available_balance() const 
    {
        MutexGuard guard(mutex_);
        return available_balance;
    }
    int64_t get_hold_amount() const { return hold_amount; }
    const std::string &get_account_status() const { return account_status; }
    const std::string &get_account_type() const { return account_type; }
    const std::string &get_account_created_at() const { return account_created_at; }
    const std::string &get_account_updated_at() const { return account_updated_at; }
    //revisit after getAll -- #
    const HashTable<int, std::shared_ptr<Transaction>>& get_transactions() const { return transactions; }

   
    pthread_mutex_t &getMutex()
    {
        return mutex_;
    }

    bool deposit(int64_t amountCents)
    {
        if (amountCents <= 0)
        {
            throw std::invalid_argument("Deposit amount must be positive");
        }
        MutexGuard guard(mutex_);
        actual_balance += amountCents;
        available_balance += amountCents;
        return true;
    }

    bool withdraw(int64_t amountCents)
    {
        if (amountCents <= 0)
        {
            throw std::invalid_argument("Withdrawal amount must be positive");
        }
        MutexGuard guard(mutex_);
        if (amountCents > available_balance)
        {
            return false;
        }
        actual_balance -= amountCents;
        available_balance -= amountCents;
        return true;
    }

    void depositUnlocked(int64_t amountCents)
    {
        if (amountCents <= 0)
        {
            throw std::invalid_argument("Deposit amount must be positive");
        }
        actual_balance += amountCents;
        available_balance += amountCents;
    }

    bool withdrawUnlocked(int64_t amountCents)
    {
        if (amountCents <= 0)
        {
            throw std::invalid_argument("Withdrawal amount must be positive");
        }
        if (amountCents > available_balance)
        {
            return false;
        }
        actual_balance -= amountCents;
        available_balance -= amountCents;
        return true;
    }

    int64_t getBalanceUnlocked() const
    {
        return available_balance;
    }

    ~Account()
    {
        pthread_mutex_destroy(&mutex_);
    }
};

#endif