#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <stdint.h>
class Transaction;
class Account
{
    int account_id;
    std::string holder_name;
    std::vector<Transaction> transaction_log;
    double balance = 0;
    // friend class Transaction;
//  double get_balance()
//     {
//         return balance;
//     }
//     bool debit_account(double amount)
    // {
//         balance -= amount;
//         return true;
//     }
//     bool credit_account(double amount)
//     {
//         balance += amount;
//         return true;
//     }
//     void update_balance(double amount)
//     {
//         balance = amount;
//     }
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
    int transaction_id;
    double amount;
    std::string status;
    std::string remarks;
    std::string type;
    std::string error_message;
    int account_src;
    int account_destination;
    uint64_t timestampUs;

    public:


    // void withdraw(Account &acc)
    // {
    //     if(amount > acc.get_balance())
    //     {
    //         status = "Failed";
    //         error_message = "Insufficient Funds";
    //     }
    //     else
    //     {
    //         acc.debit_account(amount);
    //         status ="Success";
    //     }

    // }
    // void deposit(Account &acc)
    // {
    //     acc.credit_account(amount);
    //     status = "Success";
    // }
    // void transfer(Account &src ,Account &dest)
    // {
    //     if(amount > src.get_balance())
    //     {
    //         status = "Failed";
    //         error_message = "Insufficient Balance";
    //     }
    //     else
    //     {
    //         double src_balance = src.get_balance();
    //         double dest_balance = dest.get_balance();
    //         if(src.debit_account(amount)== true && dest.credit_account(amount)== true)
    //         {
    //             status = "Success";
    //         }
    //         else
    //         {
    //             src.update_balance(src_balance);
    //             dest.update_balance(dest_balance);

    //         }
            
    //     }
    // }


};