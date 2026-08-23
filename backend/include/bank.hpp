#ifndef BANK_HPP
#define BANK_HPP

#include "hashtable.hpp"
#include "database_loader.hpp"
#include "transaction_log.hpp"
#include "transaction_request.hpp"
#include "response_queue.hpp"
#include <memory>
#include <string>
#include <vector>
#include <json/json.h>

class Bank {
private:
    Load_DB& db_;
    HashTable<int, int> accountIdToUserId_;   // secondary index accountId -> userId
    TransactionLog log_;

       bool findAccount(int accountId, std::shared_ptr<Account>& outAcc) {
        int userId;
        if (accountIdToUserId_.find(accountId, userId)) {
            std::shared_ptr<User> user;
            if (db_.getBankDb().find(userId, user) && user->getAccountsRef().find(accountId, outAcc)) {
                return true;
            }
        }

        // NOTE: this used to fall back to db_.refreshAccountByAccountId() for
        // accounts created after server startup, but that method no longer
        // exists on Load_DB. Accounts not in the initial in-memory cache
        // (built once in the constructor) won't be found until the server
        // restarts. Ask your friend if there's a new equivalent.
        return false;
    }
    


public:
    explicit Bank(Load_DB& db) : db_(db) {
        for (auto& [userId, user] : db_.getBankDb().getAll()) {
            for (auto& [accountId, acc] : user->getAccountsRef().getAll()) {
                accountIdToUserId_.insert(accountId, userId);
            }
        }
    }

    Load_DB& getDb() { return db_; }
    const TransactionLog& getTransactionLog() const { return log_; }

    ~Bank() = default;
    Bank(const Bank&) = delete;
    Bank& operator=(const Bank&) = delete;

    bool getBalance(int accountId, long long& outBalanceCents) {
        std::shared_ptr<Account> acc;
        if (!findAccount(accountId, acc)) return false;
        outBalanceCents = acc->get_actual_balance();
        return true;
    }

    bool accountExists(int accountId) {
        return accountIdToUserId_.contains(accountId);
    }

    bool applyMemoryUpdate(const TransactionRequest& t)
    {
        std::shared_ptr<Account> acc;
        if (!findAccount(t.account_id, acc)) return false;

        if (t.transaction_type == "DEPOSIT") {
            MutexGuard guard(acc->getMutex());
            acc->depositUnlocked(t.transaction_amount);
            return true;
        }
        else if (t.transaction_type == "WITHDRAW") {
            MutexGuard guard(acc->getMutex());
            acc->withdrawUnlocked(t.transaction_amount);
            return true;
        }
        else if (t.transaction_type == "TRANSFER") {
            if (!t.to_account.has_value()) return false;
            std::shared_ptr<Account> to;
            if (!findAccount(t.to_account.value(), to)) return false;

            Account* first  = (t.account_id < t.to_account.value()) ? acc.get() : to.get();
            Account* second = (t.account_id < t.to_account.value()) ? to.get()  : acc.get();

            pthread_mutex_lock(&first->getMutex());
            pthread_mutex_lock(&second->getMutex());

            acc->withdrawUnlocked(t.transaction_amount);
            to->depositUnlocked(t.transaction_amount);

            pthread_mutex_unlock(&second->getMutex());
            pthread_mutex_unlock(&first->getMutex());
            return true;
        }
        return false;
    }
};
#endif
