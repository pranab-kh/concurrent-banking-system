#pragma once
#include <iostream>
#include <string>
#include <vector>

class Account
{
    int account_id;
    std::string holder_name;
    std::vector<Transaction> transaction_log;
    double balance;

public:
    // Account creation
    Account()
    {
    }

};

class User
{
    std::string name;
    std::string telephone;
    std::string address;
    std::vector<Account> accounts;
};

class Transaction
{
    

};