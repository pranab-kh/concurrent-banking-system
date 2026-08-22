#ifndef WEB_SOCKET_CONTROLLERS_HPP
#define WEB_SOCKET_CONTROLLERS_HPP

#include <drogon/WebSocketController.h>
#include "transaction_request.hpp"
#include "request_queue.hpp"
#include <variant>
#include <optional>
#include <json/json.h>
#include <sstream>
#include <string>
#include <vector>

#include "worker_pool.hpp"

using Authentication = std::variant<LoginRequest, AccountCreationRequest>;

inline RequestQueue<Authentication> AuthenticationQueue;
inline RequestQueue<TransactionRequest> TransactionQueue;
// not good practice to initialize obj in header file crash if multiple inclusion
//inline to solve that better to instantinate the object in main later 

inline JobHub* globalJobHub = nullptr;

// Read-only handle for admin/reporting endpoints (Admin_Controller below).
// Points at the same Bank instance WorkerPool uses to process transactions,
// but Admin_Controller only ever calls the const/read-only accessors on it
// (getDb(), getTransactionLog()) — it never calls deposit()/withdraw()/
// transfer(), so this doesn't introduce a second, competing way to move
// money. Set once in server_main.cpp, same lifetime pattern as globalJobHub.
inline Bank* globalBank = nullptr;

class Authentication_Controller : public drogon::WebSocketController<Authentication_Controller>
{
public:
    // Register path mapping
    WS_PATH_LIST_BEGIN
    WS_PATH_ADD("/login");          // Explicit routing path for auth streams
    WS_PATH_ADD("/create_account"); // Explicit routing path for auth streams
    WS_PATH_LIST_END

    std::optional<Authentication> Authentication_Parser(std::string &message,const drogon::WebSocketConnectionPtr &conn, const std::string &path)
    {
        // parsing function here
        try
        {
            if (path == "/login")
            {
            try
            {
            Json::CharReaderBuilder builder;
            Json::Value json;
            std::string errors;

            std::istringstream stream(message);

            if (!Json::parseFromStream(builder, stream, &json, &errors))
            {
                std::cerr << "JSON parsing failed: " << errors << std::endl;
                return std::nullopt;
            }

            // Check required fields
            if (!json.isMember("user_id") ||
                !json["user_id"].isInt())
            {
                std::cerr << "Missing or invalid user_id" << std::endl;
                return std::nullopt;
            }

            if (!json.isMember("password") ||
                !json["password"].isString())
            {
                std::cerr << "Missing or invalid password" << std::endl;
                return std::nullopt;
            }

            // Create LoginRequest
            LoginRequest login;

            login.user_id = json["user_id"].asInt();
            login.password = json["password"].asString();

            // Store WebSocket connection
            login.connection = conn;

            return login;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Login parsing error: "
                    << e.what() << std::endl;

            return std::nullopt;
        }
    }
            else if (path == "/create_account")
            {
            try
            {
            Json::CharReaderBuilder builder;
            Json::Value json;
            std::string errors;

            std::istringstream stream(message);

            if (!Json::parseFromStream(builder, stream, &json, &errors))
            {
                std::cerr << "JSON parsing failed: " << errors << std::endl;
                return std::nullopt;
            }

            // Required string fields
            static const std::vector<std::string> requiredStringFields = {
                "full_name", "address", "mobile", "email",
                "gender", "nid", "account_type", "password"
            };

            for (const auto &field : requiredStringFields)
            {
                if (!json.isMember(field) || !json[field].isString())
                {
                    std::cerr << "Missing or invalid " << field << std::endl;
                    return std::nullopt;
                }
            }

            AccountCreationRequest accountCreation;

            // user_id is optional: present + int -> existing user adding
            // another account; absent -> brand new user (backend assigns
            // a fresh user_id). Matches AccountCreationRequest's
            // std::optional<int> user_id.
            if (json.isMember("user_id") && !json["user_id"].isNull())
            {
                if (!json["user_id"].isInt())
                {
                    std::cerr << "Invalid user_id" << std::endl;
                    return std::nullopt;
                }
                accountCreation.user_id = json["user_id"].asInt();
            }
            else
            {
                accountCreation.user_id = std::nullopt;
            }

            accountCreation.full_name = json["full_name"].asString();
            accountCreation.address = json["address"].asString();
            accountCreation.mobile = json["mobile"].asString();
            accountCreation.email = json["email"].asString();
            accountCreation.gender = json["gender"].asString();
            accountCreation.nid = json["nid"].asString();
            accountCreation.account_type = json["account_type"].asString();
            accountCreation.password = json["password"].asString();

            accountCreation.connection = conn;

            return accountCreation;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Account creation parsing error: "
                    << e.what() << std::endl;

            return std::nullopt;
        }
    }
            else
            {
                std::cerr << "Unknown authentication path: " << path << std::endl;
                return std::nullopt;
            }
        }
            catch (...)
            {
                return std::nullopt;
            }
    }

    void handleNewConnection(const drogon::HttpRequestPtr &req, const drogon::WebSocketConnectionPtr &conn) override
    {
        // generally rate limiting done here also used for cookies verification and close connection
        conn->setContext(std::make_shared<std::string>(req->path()));
        std::cout << " Websocket Connection Established" << std::endl;
    }

    // void handleNewMessage(const drogon::WebSocketConnectionPtr &conn, std::string &&message, const drogon::WebSocketMessageType &type) override
    // {
    //     // call parsing function here
    //     // enqueue in request queue here
    //     if (type == drogon::WebSocketMessageType::Text)
    //     {

    //         auto path = conn->getContext<std::string>();
    //         if (!path)
    //         {
    //             conn->send("ERROR_INTERNAL_STATE");
    //             return;
    //         }

    //         auto parsed_msg = Authentication_Parser(message,conn, *path);

    //         if (parsed_msg == std::nullopt)
    //         {
    //             std::cout << "Failed Parsing" << std::endl;
    //             conn->send("ERROR_BAD_STRUCTURE");
    //             return;
    //         }

    //         bool enqueue_success = AuthenticationQueue.push(parsed_msg.value());

    //         if (!enqueue_success)
    //         {
    //             std::cout << "QUEUE FULL" << std::endl;
    //             conn->send("SERVER_BUSY");
    //             return;
    //         }
    //     }
    // }

    void handleNewMessage(
    const drogon::WebSocketConnectionPtr &conn,
    std::string &&message,
    const drogon::WebSocketMessageType &type) override
    {
        if (type != drogon::WebSocketMessageType::Text)
            return;

    auto path = conn->getContext<std::string>();

    if (!path)
    {
        conn->send("ERROR_INTERNAL_STATE");
        return;
    }

    // Parse the JSON message
    auto parsed_msg = Authentication_Parser(
        message,
        conn,
        *path
    );

    if (!parsed_msg.has_value())
    {
        std::cout << "Failed Parsing" << std::endl;
        conn->send("ERROR_BAD_STRUCTURE");
        return;
    }

    // Make sure JobHub exists
    if (globalJobHub == nullptr)
    {
        std::cerr << "JobHub is not initialized!" << std::endl;
        conn->send("SERVER_ERROR");
        return;
    }

    // Currently handling LOGIN
    if (std::holds_alternative<LoginRequest>(parsed_msg.value()))
    {
        LoginRequest login =
            std::get<LoginRequest>(parsed_msg.value());

        globalJobHub->pushLogin(login);

        std::cout << "Login request added to JobHub"
                  << std::endl;
    }
    else if (std::holds_alternative<AccountCreationRequest>(parsed_msg.value()))
    {
        AccountCreationRequest accountReq =
            std::get<AccountCreationRequest>(parsed_msg.value());

        globalJobHub->pushAccountCreation(accountReq);

        std::cout << "Account creation request added to JobHub"
                  << std::endl;
    }
    else
    {
        conn->send("ERROR_BAD_STRUCTURE");
        return;
    }
}
    void handleConnectionClosed(const drogon::WebSocketConnectionPtr &conn) override
    {
        std::cout << "Websocket Connection Closed" << std::endl;
    }
};

class Transaction_Controller : public drogon::WebSocketController<Transaction_Controller>
{
public:
    WS_PATH_LIST_BEGIN
    WS_PATH_ADD("/transaction");
    WS_PATH_LIST_END

    std::optional<TransactionRequest> Transaction_Parser(std::string &message,const drogon::WebSocketConnectionPtr &conn)
    {
        try
        {
            Json::CharReaderBuilder builder;
            Json::Value json;
            std::string errors;

            std::istringstream stream(message);

            if (!Json::parseFromStream(builder, stream, &json, &errors))
            {
                std::cerr << "JSON parsing failed: " << errors << std::endl;
                return std::nullopt;
            }

            if (!json.isMember("account_id") || !json["account_id"].isInt())
            {
                std::cerr << "Missing or invalid account_id" << std::endl;
                return std::nullopt;
            }

            if (!json.isMember("transaction_type") || !json["transaction_type"].isString())
            {
                std::cerr << "Missing or invalid transaction_type" << std::endl;
                return std::nullopt;
            }

            if (!json.isMember("transaction_amount") ||
                !(json["transaction_amount"].isInt() || json["transaction_amount"].isInt64()))
            {
                std::cerr << "Missing or invalid transaction_amount" << std::endl;
                return std::nullopt;
            }

            TransactionRequest transaction;
            transaction.account_id = json["account_id"].asInt();
            transaction.transaction_type = json["transaction_type"].asString();
            transaction.transaction_amount = json["transaction_amount"].asInt64();
            transaction.remarks = (json.isMember("remarks") && json["remarks"].isString())
                                       ? json["remarks"].asString()
                                       : "";

            if (json.isMember("to_account") && !json["to_account"].isNull())
            {
                if (!json["to_account"].isInt())
                {
                    std::cerr << "Invalid to_account" << std::endl;
                    return std::nullopt;
                }
                transaction.to_account = json["to_account"].asInt();
            }
            else
            {
                transaction.to_account = std::nullopt;
            }

            if (json.isMember("from_account") && !json["from_account"].isNull())
            {
                if (!json["from_account"].isInt())
                {
                    std::cerr << "Invalid from_account" << std::endl;
                    return std::nullopt;
                }
                transaction.from_account = json["from_account"].asInt();
            }
            else
            {
                transaction.from_account = std::nullopt;
            }

            transaction.connection = conn;
            return transaction;
        }
        catch (const std::exception &e)
        {
            std::cerr << "Transaction parsing error: " << e.what() << std::endl;
            return std::nullopt;
        }
        catch (...)
        {
            return std::nullopt;
        }
    }

    void handleNewConnection(const drogon::HttpRequestPtr &req, const drogon::WebSocketConnectionPtr &conn) override
    {
        // generally rate limiting done here also used for cookies verification and close connection
        conn->setContext(std::make_shared<std::string>(req->path()));
        std::cout << " Websocket Connection Established" << std::endl;
    }
    void handleNewMessage(const drogon::WebSocketConnectionPtr &conn, std::string &&message, const drogon::WebSocketMessageType &type) override
    {
        // call parsing function here
        // enqueue in request queue here
        if (type == drogon::WebSocketMessageType::Text)
        {
            auto parsed_msg = Transaction_Parser(message,conn);
            if (parsed_msg == std::nullopt)
            {
                std::cout << "Error Parsing" << std::endl;
                conn->send("ERR_BAD_STRUCTURE");
                return;
            }

            if (globalJobHub == nullptr)
            {
                std::cerr << "JobHub is not initialized!" << std::endl;
                conn->send("SERVER_ERROR");
                return;
            }

            // NOTE: this used to push into the standalone `TransactionQueue`
            // declared at the top of this file, which nothing ever reads
            // from (WorkerPool only pulls from JobHub). Route it there
            // instead, same as login/account-creation above.
            globalJobHub->pushTransaction(parsed_msg.value());
        }
    }

    void handleConnectionClosed(const drogon::WebSocketConnectionPtr &conn) override
    {
        std::cout << "Websocket Connection Closed" << std::endl;
    }
};

// Read-only admin reporting endpoint. No authentication/authorization is
// enforced here — see the caveat this was flagged with when added. Anyone
// who can open a WebSocket to this path can read (but not modify) every
// user/account/transaction. Do not treat this as a real access boundary.
//
// On connect, immediately pushes one JSON snapshot and closes nothing —
// the connection stays open so the client (Admin Panel) can request a
// fresh snapshot again later by sending any text message (e.g. "refresh").
class Admin_Controller : public drogon::WebSocketController<Admin_Controller>
{
public:
    WS_PATH_LIST_BEGIN
    WS_PATH_ADD("/admin/users");
    WS_PATH_ADD("/admin/transactions");
    WS_PATH_LIST_END

    static Json::Value buildUsersSnapshot()
    {
        Json::Value root;
        root["type"] = "USERS_SNAPSHOT";
        Json::Value users(Json::arrayValue);

        if (globalBank == nullptr)
        {
            root["status"] = "error";
            root["message"] = "Bank not initialized";
            return root;
        }

        for (auto &[userId, user] : globalBank->getDb().getBankDb().getAll())
        {
            Json::Value u;
            u["user_id"] = userId;
            u["full_name"] = user->get_full_name();
            u["mobile"] = user->get_mobile();
            u["email"] = user->get_email();
            u["address"] = user->get_address();
            u["gender"] = user->get_gender();
            u["nid"] = user->get_nid();
            u["login_status"] = user->get_login_status();

            Json::Value accounts(Json::arrayValue);
            for (auto &[accId, acc] : user->getAccountsRef().getAll())
            {
                Json::Value a;
                a["account_id"] = accId;
                a["account_type"] = acc->get_account_type();
                a["account_status"] = acc->get_account_status();
                a["actual_balance_cents"] = static_cast<Json::Int64>(acc->get_actual_balance());
                a["available_balance_cents"] = static_cast<Json::Int64>(acc->get_available_balance());
                accounts.append(a);
            }
            u["accounts"] = accounts;

            users.append(u);
        }

        root["status"] = "success";
        root["users"] = users;
        return root;
    }

    static Json::Value buildTransactionsSnapshot()
    {
        Json::Value root;
        root["type"] = "TRANSACTIONS_SNAPSHOT";
        Json::Value txns(Json::arrayValue);

        if (globalBank == nullptr)
        {
            root["status"] = "error";
            root["message"] = "Bank not initialized";
            return root;
        }

        // NOTE: this reflects transactions recorded since this server
        // process started (TransactionLog is in-memory, not the DB-joined
        // history loaded per-account at startup). Good enough for "live
        // activity monitoring"; if you also need full historical
        // transactions from before the server started, that needs a
        // separate merge with each Account::get_transactions() — left out
        // here to keep this addition small and unambiguous.
        for (auto &t : globalBank->getTransactionLog().getAllTransactions())
        {
            Json::Value j;
            j["transaction_id"] = t->get_transaction_id();
            j["transaction_type"] = t->get_transaction_type();
            j["amount_cents"] = static_cast<Json::Int64>(t->get_transaction_amount());
            j["status"] = t->get_transaction_status();

            if (t->get_from_account().has_value())
                j["from_account"] = t->get_from_account().value();
            if (t->get_to_account().has_value())
                j["to_account"] = t->get_to_account().value();

            txns.append(j);
        }

        root["status"] = "success";
        root["transactions"] = txns;
        return root;
    }

    static std::string writeCompact(const Json::Value &v)
    {
        Json::StreamWriterBuilder writer;
        writer["indentation"] = "";
        return Json::writeString(writer, v);
    }

    void handleNewConnection(const drogon::HttpRequestPtr &req,
                              const drogon::WebSocketConnectionPtr &conn) override
    {
        std::cout << "Admin WebSocket Connection Established: " << req->path() << std::endl;

        if (req->path() == "/admin/users")
            conn->send(writeCompact(buildUsersSnapshot()));
        else if (req->path() == "/admin/transactions")
            conn->send(writeCompact(buildTransactionsSnapshot()));
    }

    void handleNewMessage(const drogon::WebSocketConnectionPtr &conn,
                           std::string &&message,
                           const drogon::WebSocketMessageType &type) override
    {
        // Any incoming text message is treated as a request to resend the
        // current snapshot (simplest possible "refresh" — no request body
        // parsing needed since there's nothing to parameterize yet).
        if (type != drogon::WebSocketMessageType::Text)
            return;

        // WebSocketConnectionPtr doesn't directly expose which WS_PATH_ADD
        // path this connection came in on after the handshake, so we send
        // both snapshots on any message; the Qt client's two separate
        // sockets each only look at the payload's "type" field they care
        // about and can ignore the other.
        conn->send(writeCompact(buildUsersSnapshot()));
        conn->send(writeCompact(buildTransactionsSnapshot()));
    }

    void handleConnectionClosed(const drogon::WebSocketConnectionPtr &conn) override
    {
        std::cout << "Admin WebSocket Connection Closed" << std::endl;
    }
};

#endif