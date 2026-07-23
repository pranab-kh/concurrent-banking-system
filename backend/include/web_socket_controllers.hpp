#ifndef WEB_SOCKET_CONTROLLERS_HPP
#define WEB_SOCKET_CONTROLLERS_HPP

#include <drogon/WebSocketController.h>
#include "transaction_request.hpp"
#include "request_queue.hpp"
#include <variant>
#include <optional>
#include <string>

using Authentication = std::variant<LoginRequest, AccountCreationRequest>;

inline RequestQueue<Authentication> AuthenticationQueue;
inline RequestQueue<TransactionRequest> TransactionQueue;
// not good practice to initialize obj in header file crash if multiple inclusion
//inline to solve that better to instantinate the object in main later 
class Authentication_Controller : public drogon::WebSocketController<Authentication_Controller>
{
public:
    // Register path mapping
    WS_PATH_LIST_BEGIN
    WS_PATH_ADD("/login");          // Explicit routing path for auth streams
    WS_PATH_ADD("/create_account"); // Explicit routing path for auth streams
    WS_PATH_LIST_END

    std::optional<Authentication> Authentication_Parser(std::string &message, const std::string &path)
    {
        // parsing function here
        try
        {
            if (path == "/login")
            {
                LoginRequest login;
                // login parsing logic here
                // add all parsed data in login

                return login;
            }
            else if (path == "/create_account")
            {
                AccountCreationRequest create_account;
                // create_account parsing logic here
                //  add all parsed data in create_account

                return create_account;
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

    void handleNewMessage(const drogon::WebSocketConnectionPtr &conn, std::string &&message, const drogon::WebSocketMessageType &type) override
    {
        // call parsing function here
        // enqueue in request queue here
        if (type == drogon::WebSocketMessageType::Text)
        {

            auto path = conn->getContext<std::string>();
            if (!path)
            {
                conn->send("ERROR_INTERNAL_STATE");
                return;
            }

            auto parsed_msg = Authentication_Parser(message, *path);

            if (parsed_msg == std::nullopt)
            {
                std::cout << "Failed Parsing" << std::endl;
                conn->send("ERROR_BAD_STRUCTURE");
                return;
            }

            bool enqueue_success = AuthenticationQueue.enqueue(parsed_msg.value());

            if (!enqueue_success)
            {
                std::cout << "QUEUE FULL" << std::endl;
                conn->send("SERVER_BUSY");
                return;
            }
        }
    }

    void handleConnectionClosed(const drogon::WebSocketConnectionPtr &) override
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

    std::optional<TransactionRequest> Transaction_Parser(std::string &message)
    {
        try
        {
            TransactionRequest transaction;
            // transaction parsing here
            return transaction;
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
            auto parsed_msg = Transaction_Parser(message);
            if (parsed_msg == std::nullopt)
            {
                std::cout << "Error Parsing" << std::endl;
                conn->send("ERR_BAD_STRUCTURE");
                return;
            }

            bool enqueue_success = TransactionQueue.enqueue(parsed_msg.value());
            if (!enqueue_success)
            {
                conn->send("SERVER_BUSY");
                return;
            }
        }
    }

    void handleConnectionClosed(const drogon::WebSocketConnectionPtr &conn) override
    {
        std::cout << "Websocket Connection Closed" << std::endl;
    }
};

#endif