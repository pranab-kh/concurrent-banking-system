#ifndef WORKER_POOL_HPP
#define WORKER_POOL_HPP

#include "bank.hpp"
#include "database_loader.hpp"
#include "transaction_request.hpp"
#include "request_queue.hpp"
#include "response_queue.hpp"
#include "mutex_guard.hpp"
#include <pthread.h>
#include <vector>
#include <stdexcept>

enum class JobType { LOGIN, TRANSACTION, NONE };

struct Job {
    JobType type;
    LoginRequest loginReq;
    TransactionRequest txnReq;
};

class JobHub {
private:
    circular_queue<LoginRequest> loginQueue_;
    circular_queue<TransactionRequest> transactionQueue_;
    pthread_mutex_t mutex_;
    pthread_cond_t notEmpty_;
    bool shuttingDown_ = false;

public:
    JobHub(int loginCapacity = 100, int transactionCapacity = 500)
        : loginQueue_(loginCapacity), transactionQueue_(transactionCapacity)
    {
        if (pthread_mutex_init(&mutex_, nullptr) != 0) {
            throw std::runtime_error("Failed to initialize JobHub mutex");
        }
        if (pthread_cond_init(&notEmpty_, nullptr) != 0) {
            pthread_mutex_destroy(&mutex_);
            throw std::runtime_error("Failed to initialize JobHub condition variable");
        }
    }

    ~JobHub() {
        pthread_mutex_destroy(&mutex_);
        pthread_cond_destroy(&notEmpty_);
    }

    JobHub(const JobHub&) = delete;
    JobHub& operator=(const JobHub&) = delete;

    void pushLogin(LoginRequest req) {
        MutexGuard guard(mutex_);
        if (shuttingDown_) return;
        loginQueue_.push(req);
        pthread_cond_signal(&notEmpty_);
    }

    void pushTransaction(TransactionRequest req) {
        MutexGuard guard(mutex_);
        if (shuttingDown_) return;
        transactionQueue_.push(req);
        pthread_cond_signal(&notEmpty_);
    }

    bool pop(JobType homeType, Job& outJob) {
        MutexGuard guard(mutex_);

        // sleep efficiently while both queues are empt and not shutting down
        while (loginQueue_.isEmpty() && transactionQueue_.isEmpty() && !shuttingDown_) {
            pthread_cond_wait(&notEmpty_, &mutex_);
        }

        //wake up for shutdown
        if (shuttingDown_ && loginQueue_.isEmpty() && transactionQueue_.isEmpty()) {
            return false;
        }

        if (homeType == JobType::LOGIN) {
            if (!loginQueue_.isEmpty()) { //takes from it's own login queue first if available
                outJob.type = JobType::LOGIN;
                outJob.loginReq = loginQueue_.front();
                loginQueue_.pop();
                return true;
            }
            if (!transactionQueue_.isEmpty()) { //else checks for transaction queue
                outJob.type = JobType::TRANSACTION;
                outJob.txnReq = transactionQueue_.front();
                transactionQueue_.pop();
                return true;
            }
        } else { // case for homeType == JobType::TRANSACTION
            if (!transactionQueue_.isEmpty()) {
                outJob.type = JobType::TRANSACTION;
                outJob.txnReq = transactionQueue_.front();
                transactionQueue_.pop();
                return true;
            }
            if (!loginQueue_.isEmpty()) {
                outJob.type = JobType::LOGIN;
                outJob.loginReq = loginQueue_.front();
                loginQueue_.pop();
                return true;
            }
        }

        return pop(homeType, outJob); // handles behaviour under concurrency
    }

    void shutdown() {
        MutexGuard guard(mutex_);
        shuttingDown_ = true;
        pthread_cond_broadcast(&notEmpty_);
    }
};

class WorkerPool {
private:
    JobHub& hub_;
    Bank& bank_;
    Load_DB& db_;
    ResponseQueue& responseQueue_;
    std::vector<pthread_t> workers_;

    /*
    pthread_create only accepts one void* argument but each thread needs to know both which worker pool it belongs to and its home type
    ThreadArgs bundles both into one heap-allocated object
    */
    struct ThreadArgs {
        WorkerPool* pool;
        JobType homeType;
    };

    static void* workerLoop(void* arg) //same signature required by pthread_create
    {
        ThreadArgs* args = static_cast<ThreadArgs*>(arg);
        WorkerPool* pool = args->pool;
        JobType homeType = args->homeType;
        delete args;
        pool->run(homeType); //hands over control to main worker pool logic
        return nullptr;
    }

    //main loop
    void run(JobType homeType) 
    {
        Job job;
        while (hub_.pop(homeType, job)) {
            if (job.type == JobType::LOGIN) {
                db_.login(job.loginReq);
            } else {
                TransactionResponse resp = bank_.process(job.txnReq);
                responseQueue_.push(resp);
            }
        }
    }

public:
    WorkerPool(Bank& bank, Load_DB& db, JobHub& hub, ResponseQueue& responseQueue,
               int numLoginWorkers = 2, int numTransactionWorkers = 4)
        : hub_(hub), bank_(bank), db_(db), responseQueue_(responseQueue)
    {
        for (int i = 0; i < numLoginWorkers; i++) {
            pthread_t thread;
            ThreadArgs* args = new ThreadArgs{this, JobType::LOGIN};
            if (pthread_create(&thread, nullptr, workerLoop, args) != 0) {
                delete args;
                throw std::runtime_error("Failed to create login worker thread");
            }
            workers_.push_back(thread);
        }

        for (int i = 0; i < numTransactionWorkers; i++) {
            pthread_t thread;
            ThreadArgs* args = new ThreadArgs{this, JobType::TRANSACTION};
            if (pthread_create(&thread, nullptr, workerLoop, args) != 0) {
                delete args;
                throw std::runtime_error("Failed to create transaction worker thread");
            }
            workers_.push_back(thread);
        }
    }

    ~WorkerPool() {
        hub_.shutdown();
        for (auto& t : workers_) {
            pthread_join(t, nullptr);
        }
    }

    //deleting copy operations
    WorkerPool(const WorkerPool&) = delete;
    WorkerPool& operator=(const WorkerPool&) = delete;
};

#endif