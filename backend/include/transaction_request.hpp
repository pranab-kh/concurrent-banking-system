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
struct TransactionRequest
{
    std::optional<int> to_account;
    int64_t transaction_amount;
    std::string receiver_name;
    std::string remarks; // only used for TRANSFER
    std::string transaction_type;
};

#endif