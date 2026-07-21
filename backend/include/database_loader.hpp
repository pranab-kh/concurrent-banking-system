#ifndef DATABASE_LOADER_HPP
#define DATABASE_LOADER_HPP
#include "transaction.hpp"
#include "user.hpp"
#include "account.hpp"
#include <pqxx/pqxx>
class Load_DB
{

    std::unordered_map<int, User> bank_db;

public:
    Load_DB()
    {
        try
        {
            // getting connection string to connect to neon db database
            const char *connection_string = std::getenv("BANK_DB_URL");

            // safety check if environment variable exists
            if (!connection_string)
            {
                std::cerr << "Environment Variable Not Set" << std::endl;
                return ;
            }
            // establish connection
            pqxx::connection connect_database(connection_string);

            if (connect_database.is_open())
            {
                std::cout << "Connected to Database Successfully" << std::endl;

                // only possible to read
                pqxx::nontransaction read(connect_database);

                std::string query =
                    "SELECT u.user_id,u.full_name,u.address,u.mobile,u.email,u.gender,u.nid,u.password_hash,u.user_created_at,u.user_updated_at,u.login_status,"
                    "a.account_id,a.account_holder,a.actual_balance,a.available_balance,a.hold_amount,a.account_status,a.account_type,a.account_created_at,a.account_updated_at,"
                    "t.transaction_id,t.transaction_type,t.from_account,t.to_account,t.transaction_amount,t.receiver_name,t.receiver_mobile,t.remarks,t.transaction_status,t.transaction_at "
                    "FROM User_Table u "
                    "LEFT JOIN Account_Table a on u.user_id = a.user_id "
                    "LEFT JOIN Transaction_Table t on a.account_id = t.account_id "
                    // "WHERE u.login_status = 'ONLINE' "
                    // for active account only caching
                    "ORDER BY u.user_id ASC,a.account_id ASC, t.transaction_id DESC;";

                pqxx::result res = read.exec(query);

                std::unordered_map<int, Transaction> transactions;
                std::unordered_map<int, Account> accounts;

                for (int i = 1; i < res.size(); i++)
                {
                    if (res[i - 1]["user_id"].is_null())
                    {
                        continue;
                    }

                    if (!res[i - 1]["transaction_id"].is_null())
                    {

                        int transaction_id = res[i - 1]["transaction_id"].as<int>();
                        int64_t transaction_amount = res[i - 1]["transaction_amount"].as<int64_t>();
                        std::string transaction_type = res[i - 1]["transaction_type"].as<std::string>();
                        std::string remarks = res[i - 1]["remarks"].as<std::string>();
                        std::string transaction_status = res[i - 1]["transaction_status"].as<std::string>();
                        std::string transaction_at = res[i - 1]["transaction_at"].as<std::string>();

                        // optional int these field can be null
                        std::optional<int> from_account = res[i - 1]["from_account"].as<std::optional<int>>();
                        std::optional<int> to_account = res[i - 1]["to_account"].as<std::optional<int>>();
                        std::string receiver_name = res[i - 1]["receiver_name"].is_null() ? "" : res[i - 1]["receiver_name"].as<std::string>();
                        std::string receiver_mobile = res[i - 1]["receiver_mobile"].is_null() ? "" : res[i - 1]["receiver_mobile"].as<std::string>();

                        Transaction t(transaction_id, from_account, to_account, transaction_amount, receiver_name, receiver_mobile, remarks, transaction_status, transaction_at, transaction_type);
                        transactions[transaction_id] = std::move(t);
                    }
                    if (!res[i - 1]["account_id"].is_null())
                    {

                        if (res[i - 1]["account_id"] != res[i]["account_id"])
                        {
                            int account_id = res[i - 1]["account_id"].as<int>();
                            std::string account_holder = res[i - 1]["account_holder"].as<std::string>();
                            int64_t actual_balance = res[i - 1]["actual_balance"].as<int64_t>();
                            int64_t available_balance = res[i - 1]["available_balance"].as<int64_t>();
                            int64_t hold_amount = res[i - 1]["hold_amount"].as<int64_t>();
                            std::string account_status = res[i - 1]["account_status"].as<std::string>();
                            std::string account_type = res[i - 1]["account_type"].as<std::string>();
                            std::string account_created_at = res[i - 1]["account_created_at"].as<std::string>();
                            std::string account_updated_at = res[i - 1]["account_updated_at"].as<std::string>();

                            Account a(account_id, account_holder, actual_balance, available_balance, hold_amount, account_status, account_type, account_created_at, account_updated_at, std::move(transactions));
                            accounts[account_id] = std::move(a);
                            transactions.clear();
                        }
                    }
                    if (res[i - 1]["user_id"] != res[i]["user_id"])
                    {
                        int user_id = res[i - 1]["user_id"].as<int>();
                        std::string full_name = res[i - 1]["full_name"].as<std::string>();
                        std::string address = res[i - 1]["address"].as<std::string>();
                        std::string mobile = res[i - 1]["mobile"].as<std::string>();
                        std::string email = res[i - 1]["email"].as<std::string>();
                        std::string gender = res[i - 1]["gender"].as<std::string>();
                        std::string nid = res[i - 1]["nid"].as<std::string>();
                        std::string password_hash = res[i - 1]["password_hash"].as<std::string>();
                        std::string user_created_at = res[i - 1]["user_created_at"].as<std::string>();
                        std::string user_updated_at = res[i - 1]["user_updated_at"].as<std::string>();
                        std::string login_status = res[i - 1]["login_status"].as<std::string>();

                        User u(user_id, full_name, address, mobile, email, gender, nid, password_hash, user_created_at, user_updated_at, login_status, std::move(accounts));
                        bank_db[user_id] = std::move(u);
                        transactions.clear();
                        accounts.clear();
                    }
                }

                if (!res.empty())
                {
                    size_t last = res.size() - 1;

                    // Proceed only if the last row contains a valid user profile
                    if (!res[last]["user_id"].is_null())
                    {
                        // 1. Accumulate the final transaction if it exists
                        if (!res[last]["transaction_id"].is_null())
                        {
                            int transaction_id = res[last]["transaction_id"].as<int>();
                            int64_t transaction_amount = res[last]["transaction_amount"].as<int64_t>();
                            std::string transaction_type = res[last]["transaction_type"].as<std::string>();
                            std::string remarks = res[last]["remarks"].as<std::string>();
                            std::string transaction_status = res[last]["transaction_status"].as<std::string>();
                            std::string transaction_at = res[last]["transaction_at"].as<std::string>();

                            std::optional<int> from_account = res[last]["from_account"].as<std::optional<int>>();
                            std::optional<int> to_account = res[last]["to_account"].as<std::optional<int>>();
                            std::string receiver_name = res[last]["receiver_name"].is_null() ? "" : res[last]["receiver_name"].as<std::string>();
                            std::string receiver_mobile = res[last]["receiver_mobile"].is_null() ? "" : res[last]["receiver_mobile"].as<std::string>();

                            Transaction t(transaction_id, from_account, to_account, transaction_amount, receiver_name, receiver_mobile, remarks, transaction_status, transaction_at, transaction_type);
                            transactions[transaction_id] = std::move(t);
                        }

                        // 2. Commit the final account structure to the map
                        if (!res[last]["account_id"].is_null())
                        {
                            int account_id = res[last]["account_id"].as<int>();
                            std::string account_holder = res[last]["account_holder"].as<std::string>();
                            int64_t actual_balance = res[last]["actual_balance"].as<int64_t>();
                            int64_t available_balance = res[last]["available_balance"].as<int64_t>();
                            int64_t hold_amount = res[last]["hold_amount"].as<int64_t>();
                            std::string account_status = res[last]["account_status"].as<std::string>();
                            std::string account_type = res[last]["account_type"].as<std::string>();
                            std::string account_created_at = res[last]["account_created_at"].as<std::string>();
                            std::string account_updated_at = res[last]["account_updated_at"].as<std::string>();

                            Account a(account_id, account_holder, actual_balance, available_balance, hold_amount, account_status, account_type, account_created_at, account_updated_at, std::move(transactions));
                            accounts[account_id] = std::move(a);
                        }

                        // 3. Commit the final active user profile to your bank database
                        int user_id = res[last]["user_id"].as<int>();
                        std::string full_name = res[last]["full_name"].as<std::string>();
                        std::string address = res[last]["address"].as<std::string>();
                        std::string mobile = res[last]["mobile"].as<std::string>();
                        std::string email = res[last]["email"].as<std::string>();
                        std::string gender = res[last]["gender"].as<std::string>();
                        std::string nid = res[last]["nid"].as<std::string>();
                        std::string password_hash = res[last]["password_hash"].as<std::string>();
                        std::string user_created_at = res[last]["user_created_at"].as<std::string>();
                        std::string user_updated_at = res[last]["user_updated_at"].as<std::string>();
                        std::string login_status = res[last]["login_status"].as<std::string>();

                        User u(user_id, full_name, address, mobile, email, gender, nid, password_hash, user_created_at, user_updated_at, login_status, std::move(accounts));
                        bank_db[user_id] = std::move(u);
                        // 4. Memory flush
                        transactions.clear();
                        accounts.clear();
                    }
                }
            }
        }

        catch (std::exception &e)
        {
            std::cerr << "Database Connection Failed" << e.what() << std::endl;
            return ;
        }
    }

    void display()
    {
        
        std::cout << std::left;
        std::cout << std::setw(10) << "UserID"
                  << std::setw(20) << "Name"
                  << std::setw(12) << "AcctID"
                  << std::setw(14) << "Balance"
                  << std::setw(14) << "Status"
                  << std::setw(10) << "TxnID"
                  << std::setw(14) << "Amount"
                  << std::setw(18) << "Receiver"
                  << std::setw(20) << "Remarks"
                  << std::setw(22) << "Time"
                  << std::setw(14) << "Type"
                  << std::endl;

        std::cout << std::string(140, '-') << std::endl;

        for (const auto &[u_id, u_data] : bank_db)
        {
            for (const auto &[a_id, a_data] : u_data.get_accounts())
            {
                if (a_data.get_transactions().empty())
                {
                    std::cout << std::setw(10) << u_data.get_user_id()
                              << std::setw(20) << u_data.get_full_name()
                              << std::setw(12) << a_data.get_account_id()
                              << std::setw(14) << a_data.get_actual_balance()
                              << std::setw(14) << a_data.get_account_status()
                              << "(no transactions)"
                              << std::endl;
                    continue;
                }

                for (const auto &[t_id, transaction] : a_data.get_transactions())
                {
                    std::cout << std::setw(10) << u_data.get_user_id()
                              << std::setw(20) << u_data.get_full_name()
                              << std::setw(12) << a_data.get_account_id()
                              << std::setw(14) << a_data.get_actual_balance()
                              << std::setw(14) << a_data.get_account_status()
                              << std::setw(10) << transaction.get_transaction_id()
                              << std::setw(14) << transaction.get_transaction_amount()
                              << std::setw(18) << transaction.get_receiver_name()
                              << std::setw(20) << transaction.get_remarks()
                              << std::setw(22) << transaction.get_transaction_at()
                              << std::setw(14) << transaction.get_transaction_type()
                              << std::endl;
                }
            }
        }
    }
};
#endif