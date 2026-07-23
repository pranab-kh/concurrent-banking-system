#ifndef BANK_HPP
#define BANK_HPP

#include "hashtable.hpp"
#include "account.hpp"
#include "transaction_log.hpp"
#include "transaction_request.hpp"
#include <memory>
#include <string>
#include <vector>

class Bank {
public:
    Bank() = default;
    ~Bank() = default;

    Bank(const Bank&) = delete;
    Bank& operator=(const Bank&) = delete;
    

private:
    HashTable<int, std::shared_ptr<Account>> accounts_;
    TransactionLog log_;

    bool createAccount(int accountId, const std::string& ownerName, long long initialBalanceCents = 0) 
    {
        if (accounts_.contains(accountId)) {
            return false;
        }

        auto acc = std::make_shared<Account>(accountId, ownerName, initialBalanceCents);
        accounts_.insert(accountId, acc);
        return true;
    }

    bool process(const TransactionRequest& req) 
    {
        switch (req.type) {
            case TransactionType::DEPOSIT:
                return deposit(req.accountId, req.amountCents);
            case TransactionType::WITHDRAWAL:
                return withdraw(req.accountId, req.amountCents);
            case TransactionType::TRANSFER_OUT:
                if (!req.relatedAccountId.has_value()) return false;
                return transfer(req.accountId, req.relatedAccountId.value(), req.amountCents);
            default:
                return false;
        }
    }

    bool deposit(int accountId, long long amountCents) 
    {
        std::shared_ptr<Account> acc;
        if (!accounts_.find(accountId, acc)) {
            return false;
        }
        acc->deposit(amountCents);
        log_.record(TransactionType::DEPOSIT, accountId, amountCents, acc->getBalance());
        return true;
    }

    bool withdraw(int accountId, long long amountCents) 
    {
        std::shared_ptr<Account> acc;
        if (!accounts_.find(accountId, acc)) {
            return false;
        }
        if (!acc->withdraw(amountCents)) {
            return false;   // insufficient funds
        }
        log_.record(TransactionType::WITHDRAWAL, accountId, amountCents, acc->getBalance());
        return true;
    }

    bool getBalance(int accountId, long long& outBalanceCents) {
        std::shared_ptr<Account> acc;
        if (!accounts_.find(accountId, acc)) 
        {
            return false;
        }
        outBalanceCents = acc->getBalance();
        return true;
    }

    bool accountExists(int accountId) 
    {
        return accounts_.contains(accountId);
    }

    std::vector<Transaction> getTransactionHistory(int accountId) const 
    {
        return log_.getHistoryForAccount(accountId);
    }

    bool transfer(int fromId, int toId, long long amountCents) {
        if (amountCents <= 0 || fromId == toId) return false;

        std::shared_ptr<Account> from, to;
        if (!accounts_.find(fromId, from)) return false;
        if (!accounts_.find(toId, to)) return false;

        // Lock in consistent order: lower account ID first
        Account* first  = (fromId < toId) ? from.get() : to.get();
        Account* second = (fromId < toId) ? to.get()   : from.get();

        pthread_mutex_lock(&first->getMutex());
        pthread_mutex_lock(&second->getMutex());

        bool success = false;
        if (from->getBalanceUnlocked() >= amountCents) {
            from->withdrawUnlocked(amountCents);
            to->depositUnlocked(amountCents);
            success = true;
        }

        long long fromBalance = from->getBalanceUnlocked();
        long long toBalance = to->getBalanceUnlocked();

        pthread_mutex_unlock(&second->getMutex());
        pthread_mutex_unlock(&first->getMutex());

        if (success) {
            log_.record(TransactionType::TRANSFER_OUT, fromId, amountCents, fromBalance, toId);
            log_.record(TransactionType::TRANSFER_IN, toId, amountCents, toBalance, fromId);
        }

        return success;
    }
};

#endif