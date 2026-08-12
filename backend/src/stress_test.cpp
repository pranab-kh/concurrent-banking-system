//   producer threads --push--> RequestQueue<TransactionRequest>
//                                       |
//                                  (WorkerPool)
//                                       |
//        each worker thread owns ITS OWN pqxx::connection
//                                       |
//              Load_DB::transaction(req, bank, myConn)
//                 1. runs the real SQL UPDATE/INSERT against NeonDB
//                    (Postgres's WHERE clause is the real,
//                     authoritative "sufficient funds" check)
//                 2. only on DB success, calls bank.applyMemoryUpdate(req)
//                    to mirror the change into the in-memory Account
//                                       |
//                              ResponseQueue <-- results pushed here
//                                       |
//                              main() drains + verifies
//

#include "database_loader.hpp"
#include "bank.hpp"
#include "worker_pool.hpp"
#include <iostream>
#include <vector>
#include <random>
#include <pthread.h>

//pthread_create only accepts a single void* argument.
struct ProducerArgs {
    RequestQueue<TransactionRequest>* queue;   // shared queue all producers push into
    std::vector<int>* accountIds;              // real account IDs to pick from
    int numRequests; // how many requests this thread should generate
};

void* producerFunc(void* arg) {
    ProducerArgs* args = static_cast<ProducerArgs*>(arg);

    // Each thread gets its own random engine
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> accountPicker(0, args->accountIds->size() - 1);
    std::uniform_int_distribution<int> amountPicker(100, 1000);
    std::uniform_int_distribution<int> typePicker(0, 2);

    for (int i = 0; i < args->numRequests; i++) {
        TransactionRequest req;
        req.account_id = (*args->accountIds)[accountPicker(gen)];
        req.transaction_amount = amountPicker(gen);
        req.remarks = "stress test";
        req.connection = nullptr;

        int t = typePicker(gen);
        if (t == 0) {
            req.transaction_type = "DEPOSIT";
        } else if (t == 1) {
            req.transaction_type = "WITHDRAW";
        } else {
            req.transaction_type = "TRANSFER";
            int toId;
            // pick a destination different from the source
            do {
                toId = (*args->accountIds)[accountPicker(gen)];
            } while (toId == req.account_id);
            req.to_account = toId;
        }

        bool ok = args->queue->push(req);
        if (!ok) {
            // circular_queue has a fixed capacity
            std::cerr << "WARNING: request dropped, queue full\n";
        }
    }
    return nullptr;
}

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
    std::cout << "Step 1: Load DB \n";
    Load_DB loader;
    Bank bank(loader);   // builds the accountId -> userId secondary index

    // collect every real account ID loaded, to pick from during the test
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

    // before, from memory
    bool anyNegBefore;
    long long totalBefore = sumBalances(bank, accountIds, anyNegBefore);
    std::cout << "Total balance before: " << totalBefore << " cents\n";

    RequestQueue<TransactionRequest> queue(1500);
    ResponseQueue responseQueue;

    // workerpool now also takes Load_DB&, bcs each worker thread
    // calls db.transaction(req, bank, itsOwnConnection) internally
    WorkerPool pool(bank, loader, queue, responseQueue, 8);

    const int NUM_PRODUCERS = 5;
    const int REQUESTS_PER_PRODUCER = 100;
    const int TOTAL_REQUESTS = NUM_PRODUCERS * REQUESTS_PER_PRODUCER;

    std::vector<pthread_t> producers(NUM_PRODUCERS);
    std::vector<ProducerArgs> argsList(NUM_PRODUCERS);

    std::cout << "Step 2: Launching " << NUM_PRODUCERS
              << " producer threads, " << REQUESTS_PER_PRODUCER
              << " requests each (" << TOTAL_REQUESTS << " total) \n";

    for (int i = 0; i < NUM_PRODUCERS; i++) {
        argsList[i] = { &queue, &accountIds, REQUESTS_PER_PRODUCER };
        pthread_create(&producers[i], nullptr, producerFunc, &argsList[i]);
    }
    for (auto& t : producers) {
        pthread_join(t, nullptr);   // wait until all requests have been SUBMITTED
    }
    std::cout << "All " << TOTAL_REQUESTS << " requests submitted\n";

    //drain every reponse
    long long expectedDelta = 0;
    int successCount = 0, failCount = 0;

    for (int i = 0; i < TOTAL_REQUESTS; i++) {
        TransactionResponse resp;
        if (!responseQueue.pop(resp)) {
            std::cerr << "ResponseQueue closed early — unexpected\n";
            break;
        }
        if (resp.success) {
            successCount++;
            if (resp.type == "DEPOSIT") expectedDelta += resp.amount;
            else if (resp.type == "WITHDRAW") expectedDelta -= resp.amount;
            // TRANSFER contributes 0 to expectedDelta
        } else {
            failCount++;
        }
    }

    // after, from memory
    bool anyNegAfter;
    long long totalAfterMemory = sumBalances(bank, accountIds, anyNegAfter);

    //after, from db refreshed
    std::cout << "\n Reloading DB fresh to verify persisted state \n";
    Load_DB loaderFresh;
    Bank bankFresh(loaderFresh);
    bool anyNegFresh;
    long long totalAfterDb = sumBalances(bankFresh, accountIds, anyNegFresh);

    long long expectedTotal = totalBefore + expectedDelta;

    std::cout << "\nSuccess=" << successCount << " Fail=" << failCount << "\n";
    std::cout << "Total before:            " << totalBefore << "\n";
    std::cout << "Expected total after:    " << expectedTotal << "\n";
    std::cout << "Actual total (memory):   " << totalAfterMemory << "\n";
    std::cout << "Actual total (DB reload):" << totalAfterDb << "\n";

    bool memoryOk = (totalAfterMemory == expectedTotal) && !anyNegAfter;
    bool dbOk = (totalAfterDb == expectedTotal) && !anyNegFresh;
    bool memoryMatchesDb = (totalAfterMemory == totalAfterDb);

    std::cout << "\n RESULT \n";
    if (memoryOk && dbOk && memoryMatchesDb) {
        std::cout << " PASS: Money conserved exactly, in memory & in the database. "
                   << "No negative balances. " << NUM_PRODUCERS
                   << " producer threads / 8 worker threads, each with its own DB connection, "
                   << "handled correctly. \n";
    } else {
        std::cout << " FAIL \n";
        if (!memoryOk) std::cout << "  - In-memory total mismatch\n";
        if (!dbOk) std::cout << "  - Database-reloaded total mismatch\n";
        if (!memoryMatchesDb) std::cout << "  - Memory and DB disagree with each other ("
                                         << totalAfterMemory << " vs " << totalAfterDb << ")\n";
    }

    return (memoryOk && dbOk && memoryMatchesDb) ? 0 : 1;
}