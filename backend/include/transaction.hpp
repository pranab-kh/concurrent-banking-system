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
struct Account
{
  
    int account_id;
    std::string account_holder;
    int64_t actual_balance;
    int64_t available_balance;
    int64_t hold_amount;
    std::string account_status;
    std::string account_type;
    std::string account_created_at;
    std::string account_updated_at;
    std::vector<Transaction> transactions;
    

};
struct Transaction 
{
    int transaction_id;
    int from_account;
    int to_account;
    int64_t transaction_amount;
    std::string receiver_name;
    std::string receiver_mobile;
    std::string remarks;
    std::string transaction_status;
    std::string transaction_at;
    std::string transaction_type;
   
};

#endif