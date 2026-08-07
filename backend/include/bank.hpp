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

class Bank {
private:
    Load_DB& db_;
    HashTable<int, int> accountIdToUserId_;   // secondary index accountId -> userId
    TransactionLog log_;

    bool findAccount(int accountId, std::shared_ptr<Account>& outAcc) {
        int userId;
        if (!accountIdToUserId_.find(accountId, userId)) return false;

        std::shared_ptr<User> user;
        if (!db_.getBankDb().find(userId, user)) return false;

        return user->getAccountsRef().find(accountId, outAcc);
    }

public:
    explicit Bank(Load_DB& db) : db_(db) {
        for (auto& [userId, user] : db_.getBankDb().getAll()) {
            for (auto& [accountId, acc] : user->getAccountsRef().getAll()) {
                accountIdToUserId_.insert(accountId, userId);
            }
        }
    }

    ~Bank() = default;
    Bank(const Bank&) = delete;
    Bank& operator=(const Bank&) = delete;

    bool deposit(int accountId, long long amountCents) {
        std::shared_ptr<Account> acc;
        if (!findAccount(accountId, acc)) return false;
        acc->deposit(amountCents);
        log_.record(TransactionType::DEPOSIT, accountId, amountCents, acc->get_actual_balance());
        return true;
    }

    bool withdraw(int accountId, long long amountCents) {
        std::shared_ptr<Account> acc;
        if (!findAccount(accountId, acc)) return false;
        if (!acc->withdraw(amountCents)) return false;
        log_.record(TransactionType::WITHDRAWAL, accountId, amountCents, acc->get_actual_balance());
        return true;
    }

    bool getBalance(int accountId, long long& outBalanceCents) {
        std::shared_ptr<Account> acc;
        if (!findAccount(accountId, acc)) return false;
        outBalanceCents = acc->get_actual_balance();
        return true;
    }

    bool accountExists(int accountId) {
        return accountIdToUserId_.contains(accountId);
    }

    bool transfer(int fromId, int toId, long long amountCents) {
        if (amountCents <= 0 || fromId == toId) return false;

        std::shared_ptr<Account> from, to;
        if (!findAccount(fromId, from)) return false;
        if (!findAccount(toId, to)) return false;

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

    TransactionResponse process(const TransactionRequest& req) {
        TransactionResponse resp;
        resp.requestId = 0;
        resp.type = req.transaction_type;
        resp.amount = req.transaction_amount;

        if (req.transaction_type == "DEPOSIT") {
            resp.success = deposit(req.account_id, req.transaction_amount);
            resp.message = resp.success ? "OK" : "Account not found";
        }
        else if (req.transaction_type == "WITHDRAW") {
            resp.success = withdraw(req.account_id, req.transaction_amount);
            resp.message = resp.success ? "OK" : "Insufficient funds or account not found";
        }
        else if (req.transaction_type == "TRANSFER") {
            if (!req.to_account.has_value()) {
                resp.success = false;
                resp.message = "Missing destination account";
            } else {
                resp.success = transfer(req.account_id, req.to_account.value(), req.transaction_amount);
                resp.message = resp.success ? "OK" : "Transfer failed";
            }
        }
        else {
            resp.success = false;
            resp.message = "Unknown transaction type";
        }

        long long bal;
        resp.newBalanceCents = getBalance(req.account_id, bal) ? bal : 0;
        return resp;
    }
};
#endif