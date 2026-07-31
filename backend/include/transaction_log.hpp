#ifndef TRANSACTION_LOG_HPP
#define TRANSACTION_LOG_HPP

#include "transaction.hpp"
#include "hashtable.hpp"
#include <vector>
#include <atomic>
#include <memory>
#include <optional>
#include <string>

class TransactionLog {
private:
    HashTable<int, std::shared_ptr<Transaction>> transactions_;
    std::atomic<int> nextId_;

public:
    TransactionLog() : nextId_(0) {}

    ~TransactionLog() = default;

    TransactionLog(const TransactionLog&) = delete;
    TransactionLog& operator=(const TransactionLog&) = delete;

    void record(TransactionType type, int accountId, long long amountCents, long long balanceAfterCents,
                std::optional<int> relatedAccountId = std::nullopt)
    {
        int id = nextId_++;

        std::optional<int> fromAcc =
            (type == TransactionType::WITHDRAWAL || type == TransactionType::TRANSFER_OUT)
                ? std::optional<int>(accountId) : std::nullopt;

        std::optional<int> toAcc =
            (type == TransactionType::DEPOSIT || type == TransactionType::TRANSFER_IN)
                ? std::optional<int>(accountId) : std::nullopt;

        if (type == TransactionType::TRANSFER_OUT) toAcc = relatedAccountId;
        if (type == TransactionType::TRANSFER_IN)   fromAcc = relatedAccountId;

        std::string typeStr;
        switch (type) {
            case TransactionType::DEPOSIT:      typeStr = "DEPOSIT"; break;
            case TransactionType::WITHDRAWAL:   typeStr = "WITHDRAWAL"; break;
            case TransactionType::TRANSFER_IN:  typeStr = "TRANSFER_IN"; break;
            case TransactionType::TRANSFER_OUT: typeStr = "TRANSFER_OUT"; break;
        }

        auto t = std::make_shared<Transaction>(
            id, fromAcc, toAcc, amountCents,
            "", "", "", "SUCCESS", "", typeStr
            // parameters left "" for now -- need to devide
        );

        transactions_.insert(id, t);
    }

    std::vector<std::shared_ptr<Transaction>> getHistoryForAccount(int accountId) const
    {
        std::vector<std::shared_ptr<Transaction>> result;
        for (const auto& [id, t] : transactions_.getAll()) {
            bool matchesFrom = t->get_from_account().has_value() && t->get_from_account().value() == accountId;
            bool matchesTo   = t->get_to_account().has_value()   && t->get_to_account().value()   == accountId;
            if (matchesFrom || matchesTo) {
                result.push_back(t);
            }
        }
        return result;
    }

    std::vector<std::shared_ptr<Transaction>> getAllTransactions() const
    {
        std::vector<std::shared_ptr<Transaction>> result;
        for (const auto& [id, t] : transactions_.getAll()) {
            result.push_back(t);
        }
        return result;
    }
};

#endif