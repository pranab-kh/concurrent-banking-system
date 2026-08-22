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
    std::string account_number;  
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

    // pthread_mutex_t is a plain-data struct — declaring it as a member does
    // NOT initialize it. Every constructor must explicitly call
    // pthread_mutex_init(), or the mutex is left holding whatever garbage
    // bytes happened to be on the heap. Locking/unlocking/destroying that
    // "mutex" is undefined behavior, and was the root cause of the
    // "double free or corruption" crash on shutdown: freshly-mapped memory
    // from the OS is usually zeroed (which by luck matches
    // PTHREAD_MUTEX_INITIALIZER, so it *seemed* to work at runtime), but
    // once memory gets reused the leftover bytes can make pthread's internal
    // bookkeeping treat the mutex as a robust/shared one and corrupt the
    // heap when pthread_mutex_destroy() runs across the many Account
    // objects torn down at shutdown.
    void initMutex()
    {
        if (pthread_mutex_init(&mutex_, nullptr) != 0)
        {
            throw std::runtime_error("Failed to initialize Account mutex");
        }
    }

public:
    // Default constructor
    Account()
    {
        initMutex();
    }

    // // Delete copy operations (non-copyable)
    Account(const Account &) = delete;
    Account &operator=(const Account &) = delete;

    // Move operations: give the moved-to object its OWN fresh mutex rather
    // than bit-copying the source's pthread_mutex_t (which `= default` would
    // do). Copying live mutex state is undefined behavior in the same way
    // as leaving it uninitialized -- see initMutex() above.
    Account(Account &&other) noexcept
        : account_id(other.account_id),
          account_holder(std::move(other.account_holder)),
          actual_balance(other.actual_balance),
          available_balance(other.available_balance),
          hold_amount(other.hold_amount),
          account_status(std::move(other.account_status)),
          account_type(std::move(other.account_type)),
          account_created_at(std::move(other.account_created_at)),
          account_updated_at(std::move(other.account_updated_at)),
          transactions(std::move(other.transactions))
    {
        initMutex();
    }

    Account &operator=(Account &&other) noexcept
    {
        if (this == &other) return *this;

        account_id = other.account_id;
        account_holder = std::move(other.account_holder);
        actual_balance = other.actual_balance;
        available_balance = other.available_balance;
        hold_amount = other.hold_amount;
        account_status = std::move(other.account_status);
        account_type = std::move(other.account_type);
        account_created_at = std::move(other.account_created_at);
        account_updated_at = std::move(other.account_updated_at);
        transactions = std::move(other.transactions);
        // mutex_ already initialized for `this` from construction; keep it.
        return *this;
    }

    Account(int account_id_,
            std::string account_number_, 
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
            account_number(std::move(account_number_)),  
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
        initMutex();
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

    const std::string& get_account_number() const { return account_number; }

    int64_t get_hold_amount() const { return hold_amount; }

    const std::string &get_account_status() const { return account_status; }

    const std::string &get_account_type() const { return account_type; }

    const std::string &get_account_created_at() const { return account_created_at; }

    const std::string &get_account_updated_at() const { return account_updated_at; }

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

    // Overwrites the cached balances with authoritative values just read
    // back from Postgres (e.g. the RETURNING clause of an UPDATE), instead
    // of adjusting the in-memory value by +/- amount. Postgres computed its
    // new value from whatever the row actually held at that moment, so this
    // keeps the in-memory cache honest even if the row had been edited
    // directly in the DB since it was last loaded/touched here.
    void setBalances(int64_t newActualBalance, int64_t newAvailableBalance)
    {
        MutexGuard guard(mutex_);
        actual_balance = newActualBalance;
        available_balance = newAvailableBalance;
    }

    void setBalancesUnlocked(int64_t newActualBalance, int64_t newAvailableBalance)
    {
        actual_balance = newActualBalance;
        available_balance = newAvailableBalance;
    }

    ~Account()
    {
        pthread_mutex_destroy(&mutex_);
    }
};

#endif