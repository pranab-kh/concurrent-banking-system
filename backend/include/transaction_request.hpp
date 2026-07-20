#ifndef TRANSACTION_REQUEST_HPP
#define TRANSACTION_REQUEST_HPP

#include "transaction.hpp"
#include <optional>

struct TransactionRequest {
    TransactionType type;
    int accountId;
    long long amountCents;
    std::optional<int> relatedAccountId;   // only used for TRANSFER
};

#endif