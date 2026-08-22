#ifndef WORKER_POOL_HPP
#define WORKER_POOL_HPP

#include "bank.hpp"
#include "database_loader.hpp"
#include "request_queue.hpp"
#include "response_queue.hpp"
#include <pthread.h>
#include <vector>
#include <stdexcept>
#include <pqxx/pqxx>
#include <cstdlib>
#include <iostream>

class WorkerPool {
private:
    RequestQueue<TransactionRequest>& transactionQueue_;
    RequestQueue<LoginRequest>& loginQueue_;
    Bank& bank_;
    Load_DB& db_;
    ResponseQueue& responseQueue_;
    std::vector<pthread_t> transactionWorkers_;
    std::vector<pthread_t> loginWorkers_;

    //pthread_create needs a plain function pointer
    static void* transactionWorkerLoop(void* arg) {
        WorkerPool* pool = static_cast<WorkerPool*>(arg);
        pool->runTransactionWorker();
        return nullptr;
    }

    static void* loginWorkerLoop(void* arg) {
        WorkerPool* pool = static_cast<WorkerPool*>(arg);
        pool->runLoginWorker();
        return nullptr;
    }

    // Opens ONE connection for this thread's entire life, then loops
    // forever: pop a transaction request, process it, push the result,
    // repeat. Exits only when the queue is shut down and drained.
    void runTransactionWorker() {
        const char* connStr = std::getenv("BANK_DB_URL");
        if (!connStr) {
            std::cerr << "Transaction worker: BANK_DB_URL not set, exiting\n";
            return;
        }
        pqxx::connection myConn(connStr);   // this thread's own, private connection

        TransactionRequest req;
        while (transactionQueue_.pop(req)) {
            bool ok = db_.transaction(req, bank_, myConn);   // first in db

            TransactionResponse resp;
            resp.requestId = 0;
            resp.type = req.transaction_type;
            resp.amount = req.transaction_amount;
            resp.success = ok;   // real outcome from the db write

            long long bal;
            resp.newBalanceCents = bank_.getBalance(req.account_id, bal) ? bal : 0;

            responseQueue_.push(resp);
        }
    }

    // worker never touches the transaction queue and vice versa
    void runLoginWorker() {
        const char* connStr = std::getenv("BANK_DB_URL");
        if (!connStr) {
            std::cerr << "Login worker: BANK_DB_URL not set, exiting\n";
            return;
        }
        pqxx::connection myConn(connStr);

        LoginRequest req;
        while (loginQueue_.pop(req)) {
            db_.login(req);   // login() currently reports outcome via req.connection->send(...)
        }
    }

public:
    WorkerPool(Bank& bank, Load_DB& db,
               RequestQueue<TransactionRequest>& transactionQueue,
               RequestQueue<LoginRequest>& loginQueue,
               ResponseQueue& responseQueue,
               int numTransactionWorkers = 6,
               int numLoginWorkers = 2)
        : transactionQueue_(transactionQueue), loginQueue_(loginQueue),
          bank_(bank), db_(db), responseQueue_(responseQueue)
    {
        for (int i = 0; i < numTransactionWorkers; i++) {
            pthread_t thread;
            if (pthread_create(&thread, nullptr, transactionWorkerLoop, this) != 0) {
                throw std::runtime_error("Failed to create transaction worker thread");
            }
            transactionWorkers_.push_back(thread);
        }

        for (int i = 0; i < numLoginWorkers; i++) {
            pthread_t thread;
            if (pthread_create(&thread, nullptr, loginWorkerLoop, this) != 0) {
                throw std::runtime_error("Failed to create login worker thread");
            }
            loginWorkers_.push_back(thread);
        }
    }

    ~WorkerPool() {
        transactionQueue_.shutdown();
        loginQueue_.shutdown();
        for (auto& t : transactionWorkers_) pthread_join(t, nullptr);
        for (auto& t : loginWorkers_) pthread_join(t, nullptr);
    }

    WorkerPool(const WorkerPool&) = delete;
    WorkerPool& operator=(const WorkerPool&) = delete;
};

#endif