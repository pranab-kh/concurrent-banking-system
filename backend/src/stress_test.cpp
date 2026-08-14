//   producer threads --push--> RequestQueue<TransactionRequest>
//                                       |
//                              WorkerPool (8 txn workers)
//                                       |
//        each worker thread owns ITS OWN pqxx::connection
//                                       |
//              Load_DB::transaction(req, bank, myConn)
//                 1. real SQL UPDATE (lock-ordered, deadlock-safe)
//                    + INSERT against NeonDB
//                 2. on success, mirrors the change into memory
//                    via bank.applyMemoryUpdate(req)

#include "database_loader.hpp"
#include "bank.hpp"
#include "worker_pool.hpp"
#include <iostream>
#include <vector>
#include <random>
#include <pthread.h>

// Arguments for one producer thread
struct ProducerArgs {
    RequestQueue<TransactionRequest>* queue;
    std::vector<int>* accountIds;
    int numRequests;
};

// Generates random TRANSFER requests and pushes them onto the shared queue
void* producerFunc(void* arg) {
    ProducerArgs* args = static_cast<ProducerArgs*>(arg);

    std::random_device rd;
    std::mt19937 gen(rd());   // one random engine PER THREAD — not shared
    std::uniform_int_distribution<int> accountPicker(0, args->accountIds->size() - 1);
    std::uniform_int_distribution<int> amountPicker(100, 500);

    for (int i = 0; i < args->numRequests; i++) {
        TransactionRequest req;
        req.transaction_type = "TRANSFER";
        req.transaction_amount = amountPicker(gen);
        req.remarks = "stress test transfer";
        req.connection = nullptr;

        req.account_id = (*args->accountIds)[accountPicker(gen)];
        int toId;
        do {   // destination must differ from source
            toId = (*args->accountIds)[accountPicker(gen)];
        } while (toId == req.account_id);
        req.to_account = toId;

        bool ok = args->queue->push(req);
        if (!ok) {
            std::cerr << "WARNING: request dropped, queue full\n";
        }
    }
    return nullptr;
}

// Sums actual_balance across every tracked account
long long sumBalances(Bank& bank, const std::vector<int>& accountIds, bool& anyNegative) {
    long long total = 0;
    anyNegative = false;

    for (int id : accountIds) {
        long long bal;
        if (bank.getBalance(id, bal)) {
            total += bal;
            if (bal < 0) {
                anyNegative = true;
                std::cerr << "NEGATIVE BALANCE on account " << id << ": " << bal << "\n";
            }
        }
    }
    return total;
}

int main() {
    std::cout << " Load DB \n";
    Load_DB loader;
    Bank bank(loader);

    // Collect every real account ID loaded, to transfer between
    std::vector<int> accountIds;
    for (auto& [userId, user] : loader.getBankDb().getAll()) {
        for (auto& [accId, acc] : user->get_accounts().getAll()) {
            accountIds.push_back(accId);
        }
    }

    if (accountIds.size() < 5) {
        std::cerr << "Need at least 5 accounts — found " << accountIds.size() << "\n";
        return 1;
    }
    std::cout << "Testing across " << accountIds.size() << " accounts\n";

    //sum balances of every involved account (before)
    bool anyNegBefore;
    long long totalBefore = sumBalances(bank, accountIds, anyNegBefore);
    std::cout << "Total balance before: " << totalBefore << " cents\n";

    // setting up the working pipeline
    RequestQueue<TransactionRequest> txnQueue(1500);
    RequestQueue<LoginRequest> loginQueue(100);   // unused here
    ResponseQueue responseQueue;
    WorkerPool pool(bank, loader, txnQueue, loginQueue, responseQueue, 8, 2);

    const int NUM_PRODUCERS = 5;
    const int REQUESTS_PER_PRODUCER = 100;
    const int TOTAL_REQUESTS = NUM_PRODUCERS * REQUESTS_PER_PRODUCER;

    std::vector<pthread_t> producers(NUM_PRODUCERS);
    std::vector<ProducerArgs> argsList(NUM_PRODUCERS);

    std::cout << "\n Launching " << NUM_PRODUCERS
              << " producer threads, " << REQUESTS_PER_PRODUCER
              << " TRANSFER requests each (" << TOTAL_REQUESTS << " total) \n";

    for (int i = 0; i < NUM_PRODUCERS; i++) {
        argsList[i] = { &txnQueue, &accountIds, REQUESTS_PER_PRODUCER };
        pthread_create(&producers[i], nullptr, producerFunc, &argsList[i]);
    }
    for (auto& t : producers) {
        pthread_join(t, nullptr);   // all requests SUBMITTED
    }
    std::cout << "All " << TOTAL_REQUESTS << " requests submitted\n";

    // Drain every response — this blocks until each has actually been
    // PROCESSED, basically it is like wait for all work to finish
    int successCount = 0, failCount = 0;
    for (int i = 0; i < TOTAL_REQUESTS; i++) {
        TransactionResponse resp;
        if (!responseQueue.pop(resp)) {
            std::cerr << "ResponseQueue closed early — unexpected\n";
            break;
        }
        resp.success ? successCount++ : failCount++;
    }

    //sum balances again, (from memory)
    bool anyNegAfter;
    long long totalAfterMemory = sumBalances(bank, accountIds, anyNegAfter);

    // sum balances again (from database reload)
    std::cout << "\n Reloading DB fresh to verify persisted state \n";
    Load_DB loaderFresh;
    Bank bankFresh(loaderFresh);
    bool anyNegFresh;
    long long totalAfterDb = sumBalances(bankFresh, accountIds, anyNegFresh);

    std::cout << "\nSuccess=" << successCount << " Fail=" << failCount << "\n";
    std::cout << "Total before:             " << totalBefore << "\n";
    std::cout << "Total after (memory):     " << totalAfterMemory << "\n";
    std::cout << "Total after (DB reload):  " << totalAfterDb << "\n";

    bool memoryConserved = (totalAfterMemory == totalBefore) && !anyNegAfter;
    bool dbConserved = (totalAfterDb == totalBefore) && !anyNegFresh;
    bool memoryMatchesDb = (totalAfterMemory == totalAfterDb);

    std::cout << "\n RESULT \n";
    if (memoryConserved && dbConserved && memoryMatchesDb) {
        std::cout << " PASS: Money conserved exactly, in memory AND in the database. "
                   << "No negative balances. " << NUM_PRODUCERS
                   << " producer threads / 8 transaction workers, each with its own "
                   << "DB connection, handled correctly under real concurrent load. \n";
    } else {
        std::cout << "FAIL \n";
        if (!memoryConserved) std::cout << "  - In-memory total changed (expected exactly "
                                         << totalBefore << ", got " << totalAfterMemory << ")\n";
        if (!dbConserved) std::cout << "  - DB-reloaded total changed (expected exactly "
                                     << totalBefore << ", got " << totalAfterDb << ")\n";
        if (!memoryMatchesDb) std::cout << "  - Memory and DB disagree ("
                                         << totalAfterMemory << " vs " << totalAfterDb << ")\n";
    }

    return (memoryConserved && dbConserved && memoryMatchesDb) ? 0 : 1;
}