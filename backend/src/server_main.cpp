// server_main.cpp
//
// Actual runnable server entry point for the banking backend.
//
// This file intentionally does NOT modify main.cpp (the existing CLI test
// harness written by the backend team) or any business logic in bank.hpp /
// database_loader.hpp / worker_pool.hpp / web_socket_controllers.hpp.
// It only does startup wiring:
//   1. Build the same Bank / Load_DB / WorkerPool / JobHub objects main.cpp
//      already builds.
//   2. Point globalJobHub (declared in web_socket_controllers.hpp) at the hub.
//   3. Register Authentication_Controller and Transaction_Controller with
//      Drogon so incoming WebSocket connections on /login, /create_account,
//      and /transaction actually get routed somewhere.
//   4. Start listening and run the Drogon event loop.
//
// Build/run:
//   cmake --build build --target bank_server
//   ./build/bank_server            (defaults to port 8080, or set $PORT)

#include "database_loader.hpp"
#include "bank.hpp"
#include "worker_pool.hpp"
#include "web_socket_controllers.hpp"

#include <drogon/drogon.h>
#include <iostream>
#include <cstdlib>

int main()
{
    std::cout << "Loading DB...\n";
    Load_DB loader;

    std::cout << "Initializing in-memory bank...\n";
    Bank bank(loader);

        std::cout << "Starting WorkerPool...\n";
    RequestQueue<TransactionRequest> transactionQueue;
    RequestQueue<LoginRequest> loginQueue;
    ResponseQueue responseQueue;
    WorkerPool pool(bank, loader, transactionQueue, loginQueue, responseQueue,
                     /*numTransactionWorkers=*/6, /*numLoginWorkers=*/2);

    // web_socket_controllers.hpp reads these globals to hand off parsed
    // requests from the WebSocket controllers into the same queues
    // WorkerPool's worker threads are pulling from.
    globalLoginQueue = &loginQueue;
    globalTransactionQueue = &transactionQueue;
    globalBank = &bank;
    // NOTE: WebSocketController<T> subclasses (Authentication_Controller,
    // Transaction_Controller) are auto-created and auto-registered by
    // Drogon itself the moment their translation unit is linked in (which
    // happens via the #include above). Calling
    // drogon::app().registerController(...) on them manually — as this file
    // used to — trips Drogon's isAutoCreation static_assert and fails to
    // compile. Their WS_PATH_ADD("/login") etc. macros are what register
    // the actual routes; nothing further is needed here.

    int port = 8080;
    if (const char *portEnv = std::getenv("PORT"))
    {
        port = std::stoi(portEnv);
    }

    std::cout << "Listening on 0.0.0.0:" << port << "\n";
    drogon::app().addListener("0.0.0.0", port);
    drogon::app().setThreadNum(2);

    drogon::app().run();

    return 0;
}