#ifndef TRANSACTION_REQUEST_HPP
#define TRANSACTION_REQUEST_HPP

#include "transaction.hpp"
#include <optional>

struct LoginRequest
{
    int user_id;
    std::string password_hash;
};

struct AccountCreationRequest
{
    
};
struct TransactionRequest {
    TransactionType type;
    int requestId; 
    int accountId;
    long long amountCents;
    std::optional<int> relatedAccountId;   // only used for TRANSFER
};

#endif