#include <iostream>
#include <pqxx/pqxx>
#include <cstdlib>
#include <string>
#include <cstdint>
#include <iomanip>
#include "hashtable.hpp"
#include "user.hpp"

int main()
{

    try
    {
        // getting connection string to connect to neon db database
        const char *connection_string = std::getenv("BANK_DB_URL");

        // safety check if environment variable exists
        if (!connection_string)
        {
            std::cerr << "Environment Variable Not Set" << std::endl;
            return 1;
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

            std::unordered_map<int, User> bank_db;

            for (const auto &row : res)
            {
                if (row["user_id"].is_null())
                {
                    continue;
                }

                int u_id = row["user_id"].as<int>();

                // 1. Initialize and Map the User Details safely if it's the first time seeing this User ID
                if (bank_db.find(u_id) == bank_db.end())
                {
                    bank_db[u_id].full_name = row["full_name"].as<std::string>();
                    bank_db[u_id].address = row["address"].as<std::string>();
                    bank_db[u_id].mobile = row["mobile"].as<std::string>();
                    bank_db[u_id].email = row["email"].as<std::string>();
                    bank_db[u_id].gender = row["gender"].as<std::string>();
                    bank_db[u_id].nid = row["nid"].as<std::string>();
                    bank_db[u_id].password_hash = row["password_hash"].as<std::string>();
                    bank_db[u_id].user_created_at = row["user_created_at"].as<std::string>();
                    bank_db[u_id].user_updated_at = row["user_updated_at"].as<std::string>();
                    bank_db[u_id].login_status = row["login_status"].as<std::string>();
                }

                // 2. Map Account data if the user has an account
                if (!row["account_id"].is_null())
                {
                    int a_id = row["account_id"].as<int>();

                    if (bank_db[u_id].accounts.find(a_id) == bank_db[u_id].accounts.end())
                    {
                        bank_db[u_id].accounts[a_id].account_id = a_id;
                        bank_db[u_id].accounts[a_id].account_holder = row["account_holder"].as<std::string>();
                        bank_db[u_id].accounts[a_id].actual_balance = row["actual_balance"].as<int64_t>();
                        bank_db[u_id].accounts[a_id].available_balance = row["available_balance"].as<int64_t>();
                        bank_db[u_id].accounts[a_id].hold_amount = row["hold_amount"].as<int64_t>();
                        bank_db[u_id].accounts[a_id].account_status = row["account_status"].as<std::string>();
                        bank_db[u_id].accounts[a_id].account_type = row["account_type"].as<std::string>();
                        bank_db[u_id].accounts[a_id].account_created_at = row["account_created_at"].as<std::string>();
                        bank_db[u_id].accounts[a_id].account_updated_at = row["account_updated_at"].as<std::string>();
                    }

                    // 3. Map Transaction data if a transaction exists under this account
                    if (!row["transaction_id"].is_null())
                    {
                        Transaction tx;
                        tx.transaction_id = row["transaction_id"].as<int>();
                        tx.transaction_amount = row["transaction_amount"].as<int64_t>();
                        tx.transaction_type = row["transaction_type"].as<std::string>();
                        tx.remarks = row["remarks"].as<std::string>();
                        tx.transaction_status = row["transaction_status"].as<std::string>();
                        tx.transaction_at = row["transaction_at"].as<std::string>();

                        // FIX: Using std::optional or checking nulls for fields that can be empty in your schema
                        tx.from_account = row["from_account"].is_null() ? 0 : row["from_account"].as<int>();
                        tx.to_account = row["to_account"].is_null() ? 0 : row["to_account"].as<int>();

                        tx.receiver_name = row["receiver_name"].is_null() ? "" : row["receiver_name"].as<std::string>();
                        tx.receiver_mobile = row["receiver_mobile"].is_null() ? "" : row["receiver_mobile"].as<std::string>();

                        bank_db[u_id].accounts[a_id].transactions.push_back(tx);
                    }
                }
            }
        }
    }
        
        catch (std::exception &e)
        {
            std::cerr << "Database Connection Failed" << e.what() << std::endl;
            return 1;
        }

        return 0;
    }
