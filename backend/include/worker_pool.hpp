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

class WorkerPool {
private:
    RequestQueue<TransactionRequest>& transactionQueue_;
    Bank& bank_;
    Load_DB& db_;
    ResponseQueue& responseQueue_;
    std::vector<pthread_t> workers_;

    static void* workerLoop(void* arg) {
        WorkerPool* pool = static_cast<WorkerPool*>(arg);
        pool->run();
        return nullptr;
    }

    void run() {
        const char* connStr = std::getenv("BANK_DB_URL");
        if (!connStr) {
            std::cerr << "Worker thread: BANK_DB_URL not set, exiting\n";
            return;
        }

        pqxx::connection myConn(connStr);   // ONE connection, owned by this thread for its whole life

        TransactionRequest req;
        while (transactionQueue_.pop(req)) {
            db_.transaction(req, bank_, myConn);

            TransactionResponse resp;
            resp.requestId = 0;
            resp.type = req.transaction_type;
            resp.amount = req.transaction_amount;
            long long bal;
            resp.success = bank_.getBalance(req.account_id, bal);
            resp.newBalanceCents = resp.success ? bal : 0;
            responseQueue_.push(resp);
        }
    }

public:
    WorkerPool(Bank& bank, Load_DB& db, RequestQueue<TransactionRequest>& transactionQueue,
               ResponseQueue& responseQueue, int numWorkers = 8)
        : transactionQueue_(transactionQueue), bank_(bank), db_(db), responseQueue_(responseQueue)
    {
        for (int i = 0; i < numWorkers; i++) {
            pthread_t thread;
            if (pthread_create(&thread, nullptr, workerLoop, this) != 0) {
                throw std::runtime_error("Failed to create worker thread");
            }
            workers_.push_back(thread);
        }
    }

    ~WorkerPool() {
        transactionQueue_.shutdown();
        for (auto& t : workers_) {
            pthread_join(t, nullptr);
        }
    }

    WorkerPool(const WorkerPool&) = delete;
    WorkerPool& operator=(const WorkerPool&) = delete;
};

#endif