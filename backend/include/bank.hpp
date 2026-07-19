#ifndef BANK_HPP
#define BANK_HPP

#include "hashtable.hpp"
#include "account.hpp"
#include "transaction_log.hpp"
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
};

#endif