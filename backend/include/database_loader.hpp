#ifndef DATABASE_LOADER_HPP
#define DATABASE_LOADER_HPP
#include "transaction.hpp"
#include "transaction_request.hpp"
#include "user.hpp"
#include "account.hpp"
#include <pqxx/pqxx>
#include <memory>
#include "password_hash.hpp"
#include "hashtable.hpp"
#include <json/json.h>

class Load_DB
{

    HashTable<int, std::shared_ptr<User>> bank_db;
    std::unique_ptr<pqxx::connection> connect_database;

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
                return;
            }
            // establish connection
            connect_database = std::make_unique<pqxx::connection>(connection_string);

            if (connect_database->is_open())
            {
                std::cout << "Connected to Database Successfully" << std::endl;

                // only possible to read
                pqxx::nontransaction read(*connect_database);

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
                store_users(res);
            }
        }

        catch (std::exception &e)
        {
            std::cerr << "Database Connection Failed" << e.what() << std::endl;
            return;
        }
    }

    // getter
    HashTable<int, std::shared_ptr<User>> &getBankDb()
    {
        return bank_db;
    }

    // Fetches the single user (and all their accounts) that owns the given
    // account_id, and merges it into bank_db via store_users(). Used by
    // Bank::findAccount() as a fallback when an account isn't found in the
    // in-memory cache — e.g. because it was created or funded after this
    // Load_DB/Bank was constructed at server startup. Returns false if the
    // account doesn't exist in the DB either, or on any DB error.
    bool refreshAccountByAccountId(int accountId)
    {
        try
        {
            if (!connect_database || !connect_database->is_open())
            {
                if (!establish_connection())
                    return false;
            }

            pqxx::nontransaction read(*connect_database);

            // Look up which user owns this account first, then reuse the
            // same full-profile query login()/the constructor use so
            // store_users() sees a normally-shaped result set.
            std::string ownerQuery =
                "SELECT user_id FROM Account_Table WHERE account_id = ($1);";
            pqxx::result ownerRes = read.exec_params(ownerQuery, accountId);
            if (ownerRes.size() == 0)
            {
                return false; // account genuinely doesn't exist
            }
            int userId = ownerRes[0]["user_id"].as<int>();

            std::string query =
                "SELECT u.user_id,u.full_name,u.address,u.mobile,u.email,u.gender,u.nid,u.password_hash,u.user_created_at,u.user_updated_at,u.login_status,"
                "a.account_id,a.account_holder,a.actual_balance,a.available_balance,a.hold_amount,a.account_status,a.account_type,a.account_created_at,a.account_updated_at,"
                "t.transaction_id,t.transaction_type,t.from_account,t.to_account,t.transaction_amount,t.receiver_name,t.receiver_mobile,t.remarks,t.transaction_status,t.transaction_at "
                "FROM User_Table u "
                "LEFT JOIN Account_Table a on u.user_id = a.user_id "
                "LEFT JOIN Transaction_Table t on a.account_id = t.account_id "
                "WHERE u.user_id = ($1) "
                "ORDER BY u.user_id ASC,a.account_id ASC, t.transaction_id DESC";

            pqxx::result res = read.exec_params(query, userId);
            if (res.empty())
            {
                return false;
            }

            store_users(res);
            return true;
        }
        catch (const std::exception &e)
        {
            std::cerr << "refreshAccountByAccountId ERROR: " << e.what() << std::endl;
            return false;
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

        for (const auto &[u_id, u_data] : bank_db.getAll())
        {
            for (const auto &[a_id, a_data] : u_data->get_accounts().getAll())
            {
                auto txns = a_data->get_transactions().getAll();
                if (txns.empty())
                {
                    std::cout << std::setw(10) << u_data->get_user_id()
                              << std::setw(20) << u_data->get_full_name()
                              << std::setw(12) << a_data->get_account_id()
                              << std::setw(14) << a_data->get_actual_balance()
                              << std::setw(14) << a_data->get_account_status()
                              << "(no transactions)"
                              << std::endl;
                    continue;
                }

                for (const auto &[t_id, transaction] : txns)
                {
                    std::cout << std::setw(10) << u_data->get_user_id()
                              << std::setw(20) << u_data->get_full_name()
                              << std::setw(12) << a_data->get_account_id()
                              << std::setw(14) << a_data->get_actual_balance()
                              << std::setw(14) << a_data->get_account_status()
                              << std::setw(10) << transaction->get_transaction_id()
                              << std::setw(14) << transaction->get_transaction_amount()
                              << std::setw(18) << transaction->get_receiver_name()
                              << std::setw(20) << transaction->get_remarks()
                              << std::setw(22) << transaction->get_transaction_at()
                              << std::setw(14) << transaction->get_transaction_type()
                              << std::endl;
                }
            }
        }
    }

    bool establish_connection()
    {
        const char *connection_string = std::getenv("BANK_DB_URL");

        if (!connection_string)
        {
            std::cerr << "Environment Variable Not Found!" << std::endl;
            return false;
        }

        connect_database = std::make_unique<pqxx::connection>(connection_string);
        if (!connect_database->is_open())
        {
            std::cerr << "Couldn't connect to database" << std::endl;
            return false;
        }
        return true;
    }

    void login(LoginRequest &l)
    {
        try
        {
            if (!connect_database->is_open())
            {
                establish_connection();
            }
            if (!connect_database->is_open())
            {
                std::cerr << "Couldn't connect to database" << std::endl;
                l.connection->send("Connection Error DB");
                return;
            }

            std::string hashed_password;
            std::shared_ptr<User> existingUser;
            bool userAlreadyLoaded = bank_db.find(l.user_id, existingUser);

            pqxx::work login(*connect_database);

            if (!userAlreadyLoaded)
            {
                std::string query1 =
                    "SELECT u.password_hash FROM User_Table u WHERE u.user_id = ($1);";
                pqxx::result verification = login.exec_params(query1, l.user_id);
                if (verification.size() == 0)
                {
                    std::cerr << "Invalid username of password" << std::endl;
                    l.connection->send("Invalid username or password");
                    return;
                }
                hashed_password = verification[0]["password_hash"].as<std::string>();
            }
            else
            {
                hashed_password = existingUser->get_password_hash();
            }

            bool verified = verify_password(l.password, hashed_password);
            if (!verified)
            {
                std::cerr << "Invalid username of password" << std::endl;
                l.connection->send("Invalid username of password");
                return;
            }

            std::string status = "ONLINE";
            std::string update_db =
                "UPDATE User_Table SET Login_Status = $1 WHERE User_Id = $2;";
            login.exec_params(update_db, status, l.user_id);
            login.commit();

            if (!userAlreadyLoaded)
            {
                pqxx::nontransaction read(*connect_database);
                std::string query2 =
                    "SELECT u.user_id,u.full_name,u.address,u.mobile,u.email,u.gender,u.nid,u.password_hash,u.user_created_at,u.user_updated_at,u.login_status,"
                    "a.account_id,a.account_holder,a.actual_balance,a.available_balance,a.hold_amount,a.account_status,a.account_type,a.account_created_at,a.account_updated_at,"
                    "t.transaction_id,t.transaction_type,t.from_account,t.to_account,t.transaction_amount,t.receiver_name,t.receiver_mobile,t.remarks,t.transaction_status,t.transaction_at "
                    "FROM User_Table u "
                    "LEFT JOIN Account_Table a on u.user_id = a.user_id "
                    "LEFT JOIN Transaction_Table t on a.account_id = t.account_id "
                    "WHERE u.user_id = ($1) "
                    "ORDER BY u.user_id ASC,a.account_id ASC, t.transaction_id DESC";

                pqxx::result res = read.exec_params(query2, l.user_id);
                store_users(res);
            }

            std::shared_ptr<User> u;
            if (bank_db.find(l.user_id, u))
            {
                u->set_login(status);
            }

            // Build a JSON success reply so the frontend can populate
            // accountName / accountId / accountBalanceCents (see
            // BackendClient::updateAccountInfoFromLoginMessage on the
            // Qt side, which looks for exactly these field names).
            if (l.connection)
{
    Json::Value reply;
    reply["status"] = "success";

    if (u)
    {
        reply["full_name"] = u->get_full_name();

        // Pick the first account on this user as the "primary"
        // account summary for the login reply. A user with
        // multiple accounts can fetch the rest separately.
        auto accounts = u->get_accounts().getAll();

        std::cerr << "[DEBUG] user_id=" << u->get_user_id()
                  << " accounts.size()=" << accounts.size() << std::endl;

        if (!accounts.empty())
        {
            std::cerr << "[DEBUG] accounts.front().first (key)=" << accounts.front().first
                      << " get_account_id()=" << accounts.front().second->get_account_id()
                      << std::endl;

            reply["account_id"] = accounts.front().second->get_account_id();
            reply["balance_cents"] = static_cast<Json::Int64>(
                accounts.front().second->get_actual_balance());
        }
    }

    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    std::string out = Json::writeString(writer, reply);
    l.connection->send(out);
}
        }
        catch (std::exception &e)
        {
            std::cerr << "ERROR:" << e.what() << std::endl;
            l.connection->send("ERROR");
            return;
        }
    }

    void create_account(AccountCreationRequest &a)
    {
        try
        {

            if (!connect_database->is_open())
            {
                establish_connection();
            }
            if (!connect_database->is_open())
            {
                std::cerr << "Couldn't connect to database" << std::endl;
                if (a.connection)
                    a.connection->send("Couldn't connect to database");
                return;
            }
            std::shared_ptr<User> loaded_user;
            bool user_exist = false;
            bool verified = false;
            pqxx::work user(*connect_database);
            if (a.user_id.has_value())
            {
                if (bank_db.find(a.user_id.value(), loaded_user))
                {
                    user_exist = true;
                    // check verification of password;
                }
                else
                {
                    std::string query2 =
                        "SELECT password_hash "
                        "FROM User_Table u "
                        "WHERE u.user_id = ($1);";

                    pqxx::result res = user.exec_params(query2, a.user_id.value());

                    if (res.size() != 0)
                    {
                        user_exist = true;
                        std::string password_hash = res[0]["password_hash"].as<std::string>();
                        verified = verify_password(a.password, password_hash);
                        std::cout << "password matched" << std::endl;
                    }
                }
                if (!user_exist)
                {
                    std::cerr << "Invalid username or password" << std::endl;
                    if (a.connection)
                        a.connection->send("Invalid username or password");
                    return;
                }

                if (!verified)
                {
                    std::cerr << "Invalid username or password" << std::endl;
                    if (a.connection)
                        a.connection->send("Invalid username or password");
                    return;
                }
            }
            if (!a.user_id.has_value())
            {
                bool success = hash_password(a.password);
                if (!success)
                {
                    std::cerr << "Password Hashing Failed" << std::endl;
                    if (a.connection)
                        a.connection->send("Error");
                    return;
                }
                std::string query =
                    "INSERT INTO User_Table (full_name,address,mobile,email,gender,nid,password_hash) "
                    "VALUES ($1,$2,$3,$4,$5,$6,$7) "
                    "RETURNING user_id;";

                pqxx::result res = user.exec_params(query, a.full_name, a.address, a.mobile, a.email, a.gender, a.nid, a.password);
                if (res.size() == 0)
                {
                    std::cerr << "Error creating user" << std::endl;
                    if (a.connection)
                        a.connection->send("Error creating user");
                    return;
                }
                a.user_id = res[0]["user_id"].as<int>();
            }

            std::string query3 =
                "INSERT INTO Account_Table (user_id,account_holder,account_type) "
                "VALUES($1,$2,$3);";

            auto result = user.exec_params(query3, a.user_id, a.full_name, a.account_type);
            if (result.affected_rows() == 0)
            {
                std::cerr << "Failed to insert account record" << std::endl;
                if (a.connection)
                    a.connection->send("Account creation failed");
                return;
            }

            user.commit();
            if (a.connection)
                a.connection->send("Success");
        }
        catch (std::exception &e)
        {
            std::cerr << "ERROR in create_account: " << e.what() << std::endl;
            if (a.connection)
                a.connection->send(std::string("ERROR: ") + e.what());

            return;
        }
    }

    // ------------------------------------------------------------------
    // Postgres-authoritative balance mutations.
    //
    // These replace the old, never-called Load_DB::transaction() (which
    // wrote to Postgres but had no caller — Bank::process() was the only
    // path actually wired to reply to clients, and it only updated the
    // in-memory Account, so balances never survived a restart and two
    // divergent implementations of the same three operations existed side
    // by side).
    //
    // Bank::deposit()/withdraw()/transfer() now call these directly. Each
    // one does the arithmetic in Postgres itself (actual_balance =
    // actual_balance + $1, evaluated against whatever the row currently
    // holds) and hands back the resulting balance via RETURNING, which the
    // caller then writes into the in-memory Account with
    // Account::setBalances(). That closes the other reported gap too:
    // a manual `UPDATE Account_Table SET actual_balance = ...` run
    // directly in Postgres is picked up the next time that account is
    // deposited/withdrawn/transferred, because the update reads the live
    // DB row rather than adding onto the (possibly stale) cached value.
    //
    // A plain balance check with no further transaction obviously can't
    // see a manual DB edit until the account is next touched this way —
    // there is no push/invalidation channel from Postgres back into the
    // in-memory cache in this design — but that's a much narrower gap than
    // "won't show up until the server restarts".
    bool applyDeposit(int accountId, int64_t amountCents, const std::string &remarks,
                       int64_t &outActualBalance, int64_t &outAvailableBalance)
    {
        try
        {
            if (!connect_database || !connect_database->is_open())
            {
                if (!establish_connection()) return false;
            }

            pqxx::work work(*connect_database);

            std::string deposit =
                "UPDATE Account_Table "
                "SET actual_balance = actual_balance + $1, "
                "    available_balance = available_balance + $1 "
                "WHERE account_id = $2 "
                "RETURNING actual_balance, available_balance;";

            auto result = work.exec_params(deposit, amountCents, accountId);
            if (result.affected_rows() == 0)
            {
                std::cerr << "applyDeposit: account not found: " << accountId << std::endl;
                return false;
            }

            outActualBalance = result[0]["actual_balance"].as<int64_t>();
            outAvailableBalance = result[0]["available_balance"].as<int64_t>();

            std::string transaction_log =
                "INSERT INTO Transaction_Table (account_id, transaction_type, transaction_amount, remarks) "
                "VALUES ($1, 'DEPOSIT', $2, $3);";
            work.exec_params(transaction_log, accountId, amountCents, remarks);

            work.commit();
            return true;
        }
        catch (const std::exception &e)
        {
            std::cerr << "applyDeposit ERROR: " << e.what() << std::endl;
            return false;
        }
    }

    bool applyWithdraw(int accountId, int64_t amountCents, const std::string &remarks,
                        int64_t &outActualBalance, int64_t &outAvailableBalance)
    {
        try
        {
            if (!connect_database || !connect_database->is_open())
            {
                if (!establish_connection()) return false;
            }

            pqxx::work work(*connect_database);

            std::string withdraw =
                "UPDATE Account_Table "
                "SET actual_balance = actual_balance - $1, "
                "    available_balance = available_balance - $1 "
                "WHERE account_id = $2 AND available_balance >= $1 "
                "RETURNING actual_balance, available_balance;";

            auto result = work.exec_params(withdraw, amountCents, accountId);
            if (result.affected_rows() == 0)
            {
                // Either the account doesn't exist, or funds are
                // insufficient against the DB's *current* row.
                return false;
            }

            outActualBalance = result[0]["actual_balance"].as<int64_t>();
            outAvailableBalance = result[0]["available_balance"].as<int64_t>();

            std::string transaction_log =
                "INSERT INTO Transaction_Table (account_id, transaction_type, transaction_amount, remarks) "
                "VALUES ($1, 'WITHDRAW', $2, $3);";
            work.exec_params(transaction_log, accountId, amountCents, remarks);

            work.commit();
            return true;
        }
        catch (const std::exception &e)
        {
            std::cerr << "applyWithdraw ERROR: " << e.what() << std::endl;
            return false;
        }
    }

    bool applyTransfer(int fromAccountId, int toAccountId, int64_t amountCents, const std::string &remarks,
                        int64_t &outFromActualBalance, int64_t &outFromAvailableBalance,
                        int64_t &outToActualBalance, int64_t &outToAvailableBalance)
    {
        try
        {
            if (!connect_database || !connect_database->is_open())
            {
                if (!establish_connection()) return false;
            }

            // Single DB-side transaction: both legs commit together or
            // neither does (pqxx::work rolls back automatically if we
            // return without calling commit()).
            pqxx::work work(*connect_database);

            std::string withdraw =
                "UPDATE Account_Table "
                "SET actual_balance = actual_balance - $1, "
                "    available_balance = available_balance - $1 "
                "WHERE account_id = $2 AND available_balance >= $1 "
                "RETURNING actual_balance, available_balance;";

            std::string deposit =
                "UPDATE Account_Table "
                "SET actual_balance = actual_balance + $1, "
                "    available_balance = available_balance + $1 "
                "WHERE account_id = $2 "
                "RETURNING actual_balance, available_balance;";

            auto result_1 = work.exec_params(withdraw, amountCents, fromAccountId);
            if (result_1.affected_rows() == 0)
            {
                return false; // insufficient funds or sender missing
            }

            auto result_2 = work.exec_params(deposit, amountCents, toAccountId);
            if (result_2.affected_rows() == 0)
            {
                return false; // recipient missing; withdraw above auto-rolls-back
            }

            outFromActualBalance = result_1[0]["actual_balance"].as<int64_t>();
            outFromAvailableBalance = result_1[0]["available_balance"].as<int64_t>();
            outToActualBalance = result_2[0]["actual_balance"].as<int64_t>();
            outToAvailableBalance = result_2[0]["available_balance"].as<int64_t>();

            std::string transaction_log_from =
                "INSERT INTO Transaction_Table (account_id, transaction_type, transaction_amount, remarks, to_account) "
                "VALUES ($1, 'TRANSFER', $2, $3, $4);";
            std::string transaction_log_to =
                "INSERT INTO Transaction_Table (account_id, transaction_type, transaction_amount, remarks, from_account) "
                "VALUES ($1, 'TRANSFER', $2, $3, $4);";

            work.exec_params(transaction_log_from, fromAccountId, amountCents, remarks, toAccountId);
            work.exec_params(transaction_log_to, toAccountId, amountCents, remarks, fromAccountId);

            work.commit();
            return true;
        }
        catch (const std::exception &e)
        {
            std::cerr << "applyTransfer ERROR: " << e.what() << std::endl;
            return false;
        }
    }

    void store_users(pqxx::result &res)
    {
        HashTable<int, std::shared_ptr<Transaction>> transactions;
        HashTable<int, std::shared_ptr<Account>> accounts;

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

                auto t = std::make_shared<Transaction>(transaction_id, from_account, to_account, transaction_amount, receiver_name, receiver_mobile, remarks, transaction_status, transaction_at, transaction_type);
                transactions.insert(transaction_id, t);
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

                    auto a = std::make_shared<Account>(account_id, account_holder, actual_balance, available_balance, hold_amount, account_status, account_type, account_created_at, account_updated_at, std::move(transactions));
                    accounts.insert(account_id, a);
                    transactions = HashTable<int, std::shared_ptr<Transaction>>();
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

                auto u = std::make_shared<User>(user_id, full_name, address, mobile, email, gender, nid, password_hash, user_created_at, user_updated_at, login_status, std::move(accounts));
                bank_db.insert(user_id, u);
                transactions = HashTable<int, std::shared_ptr<Transaction>>();
                accounts = HashTable<int, std::shared_ptr<Account>>();
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

                    auto t = std::make_shared<Transaction>(transaction_id, from_account, to_account, transaction_amount, receiver_name, receiver_mobile, remarks, transaction_status, transaction_at, transaction_type);
                    transactions.insert(transaction_id, t);
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

                    auto a = std::make_shared<Account>(account_id, account_holder, actual_balance, available_balance, hold_amount, account_status, account_type, account_created_at, account_updated_at, std::move(transactions));
                    accounts.insert(account_id, a);
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

                auto u = std::make_shared<User>(user_id, full_name, address, mobile, email, gender, nid, password_hash, user_created_at, user_updated_at, login_status, std::move(accounts));
                bank_db.insert(user_id, u);
                // 4. Memory flush
                transactions = HashTable<int, std::shared_ptr<Transaction>>();
                accounts = HashTable<int, std::shared_ptr<Account>>();
            }
        }
    }
};
#endif