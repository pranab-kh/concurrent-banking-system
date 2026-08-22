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

        // Cache miss: the account may have been created, or its owning
        // user loaded, after this Bank/Load_DB pair was constructed at
        // server startup (accountIdToUserId_ is only populated once, in
        // the constructor). Fall back to a targeted DB fetch instead of
        // requiring a full server restart for every new account.
        if (!db_.refreshAccountByAccountId(accountId)) {
            return false; // genuinely doesn't exist (or DB error)
        }

        // refreshAccountByAccountId() merged the owning user (and all
        // their accounts) into db_.getBankDb() via store_users(), but we
        // don't know that user's id here — scan to find which user now
        // holds this account and index it for next time.
        for (auto& [uid, user] : db_.getBankDb().getAll()) {
            std::shared_ptr<Account> acc;
            if (user->getAccountsRef().find(accountId, acc)) {
                accountIdToUserId_.insert(accountId, uid);
                outAcc = acc;
                return true;
            }
        }
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

    // Read-only accessors for admin/reporting use (e.g. Admin_Controller).
    // Neither of these lets a caller mutate money-moving state directly —
    // deposit()/withdraw()/transfer() above remain the only paths that
    // change balances — so this is safe to expose without affecting any
    // existing transaction logic.
    Load_DB& getDb() { return db_; }
    const TransactionLog& getTransactionLog() const { return log_; }

    ~Bank() = default;
    Bank(const Bank&) = delete;
    Bank& operator=(const Bank&) = delete;

    bool deposit(int accountId, long long amountCents, const std::string& remarks = "") {
        std::shared_ptr<Account> acc;
        if (!findAccount(accountId, acc)) return false;

        // Postgres is now the source of truth for the arithmetic: the
        // UPDATE below adds amountCents onto whatever the row currently
        // holds (which may already reflect a manual SQL edit made since
        // this account was last cached), then RETURNING hands back the
        // real post-update balance for us to store in memory. This is
        // what replaces the old "add to the in-memory copy, and separately
        // -- never -- write to Postgres" behavior.
        int64_t newActual, newAvailable;
        if (!db_.applyDeposit(accountId, amountCents, remarks, newActual, newAvailable)) {
            return false;
        }
        acc->setBalances(newActual, newAvailable);

        log_.record(TransactionType::DEPOSIT, accountId, amountCents, acc->get_actual_balance());
        return true;
    }

    bool withdraw(int accountId, long long amountCents, const std::string& remarks = "") {
        std::shared_ptr<Account> acc;
        if (!findAccount(accountId, acc)) return false;

        int64_t newActual, newAvailable;
        if (!db_.applyWithdraw(accountId, amountCents, remarks, newActual, newAvailable)) {
            return false; // insufficient funds (against the DB's live row) or DB error
        }
        acc->setBalances(newActual, newAvailable);

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

    bool transfer(int fromId, int toId, long long amountCents, const std::string& remarks = "") {
        if (amountCents <= 0 || fromId == toId) return false;

        std::shared_ptr<Account> from, to;
        if (!findAccount(fromId, from)) return false;
        if (!findAccount(toId, to)) return false;

        Account* first  = (fromId < toId) ? from.get() : to.get();
        Account* second = (fromId < toId) ? to.get()   : from.get();

        pthread_mutex_lock(&first->getMutex());
        pthread_mutex_lock(&second->getMutex());

        // Both legs are applied in a single Postgres transaction (see
        // Load_DB::applyTransfer) so the DB commits or rolls back atomically;
        // we hold both in-memory locks across that call so no other worker
        // thread can read/mutate either account while memory and DB are
        // momentarily out of sync mid-call.
        int64_t fromActual, fromAvailable, toActual, toAvailable;
        bool success = db_.applyTransfer(fromId, toId, amountCents, remarks,
                                          fromActual, fromAvailable, toActual, toAvailable);
        if (success) {
            from->setBalancesUnlocked(fromActual, fromAvailable);
            to->setBalancesUnlocked(toActual, toAvailable);
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
            resp.success = deposit(req.account_id, req.transaction_amount, req.remarks);
            resp.message = resp.success ? "OK" : "Account not found or database unavailable";
        }
        else if (req.transaction_type == "WITHDRAW") {
            resp.success = withdraw(req.account_id, req.transaction_amount, req.remarks);
            resp.message = resp.success ? "OK" : "Insufficient funds, account not found, or database unavailable";
        }
        else if (req.transaction_type == "TRANSFER") {
            if (!req.to_account.has_value()) {
                resp.success = false;
                resp.message = "Missing destination account";
            } else {
                resp.success = transfer(req.account_id, req.to_account.value(), req.transaction_amount, req.remarks);
                resp.message = resp.success ? "OK" : "Transfer failed";
            }
        }
        else {
            resp.success = false;
            resp.message = "Unknown transaction type";
        }

        long long bal;
        resp.newBalanceCents = getBalance(req.account_id, bal) ? bal : 0;

        // Send the result back over the WebSocket that made the request.
        // deposit()/withdraw()/transfer() above now persist to Postgres
        // themselves (via Load_DB::applyDeposit/applyWithdraw/applyTransfer)
        // before updating the in-memory Account, so this is the one and
        // only transaction path -- there's no separate DB-only
        // implementation left to drift out of sync with it.
        if (req.connection) {
            Json::Value reply;
            reply["status"] = resp.success ? "success" : "error";
            reply["type"] = resp.type;
            reply["amount"] = static_cast<Json::Int64>(resp.amount);
            reply["message"] = resp.message;
            reply["balance_cents"] = static_cast<Json::Int64>(resp.newBalanceCents);

            Json::StreamWriterBuilder writer;
            writer["indentation"] = "";
            std::string out = Json::writeString(writer, reply);
            req.connection->send(out);
        }

        return resp;
    }
};
#endif