#ifndef TRANSACTION_HPP
#define TRANSACTION_HPP

#include <ctime>
#include <optional>

enum class TransactionType 
{
    DEPOSIT,
    WITHDRAWAL,
    TRANSFER_IN,
    TRANSFER_OUT
};

struct Transaction 
{
    int transactionId;
    TransactionType type;
    int accountId;
    std::optional<int> relatedAccountId;
    long long amountCents;
    long long balanceAfterCents;
    time_t timestamp;
};

#endif