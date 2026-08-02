#ifndef TRANSACTION_REQUEST_HPP
#define TRANSACTION_REQUEST_HPP

#include "transaction.hpp"
#include <optional>
#include <drogon/drogon.h>
#include <drogon/WebSocketController.h>

struct LoginRequest
{
    int user_id;
    std::string password;
    drogon::WebSocketConnectionPtr connection;
};

struct AccountCreationRequest
{
    std::optional<int> user_id;
    std::string full_name;
    std::string address;
    std::string mobile;
    std::string email;
    std::string gender;
    std::string nid;
    std::string account_type;
    std::string password; // if user id already exist same password for verification
    drogon::WebSocketConnectionPtr connection;
};

struct TransactionRequest
{
    int account_id;
    std::string transaction_type;
    std::optional<int> to_account;
    std::optional<int> from_account;
    int64_t transaction_amount;
    std::string remarks;
    drogon::WebSocketConnectionPtr connection;
};

#endif