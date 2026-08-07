#include "database_loader.hpp"
#include "bank.hpp"
#include "worker_pool.hpp"
#include <iostream>
#include <vector>
#include <random>
#include <pthread.h>
#include <atomic>

struct ProducerArgs {
    JobHub* hub;
    std::vector<int>* accountIds;
    int numRequests;
};

void* producerFunc(void* arg) 
{
    //to select random accounts,  amounts and operation type
    ProducerArgs* args = static_cast<ProducerArgs*>(arg);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> accountPicker(0, args->accountIds->size() - 1);
    std::uniform_int_distribution<int> amountPicker(100, 2000);
    std::uniform_int_distribution<int> typePicker(0, 2);

    for (int i = 0; i < args->numRequests; i++) {
        TransactionRequest req;
        req.account_id = (*args->accountIds)[accountPicker(gen)];
        req.transaction_amount = amountPicker(gen);

        int t = typePicker(gen);
        if (t == 0) {
            req.transaction_type = "DEPOSIT";
        } else if (t == 1) {
            req.transaction_type = "WITHDRAW";
        } else {
            req.transaction_type = "TRANSFER";
            int toId;
            do {
                toId = (*args->accountIds)[accountPicker(gen)];
            } while (toId == req.account_id);
            req.to_account = toId;
        }
        // push the fully built request to shared JobHub
        args->hub->pushTransaction(req);

    }
    return nullptr;
}

int main() {
    std::cout << "Loading DB\n";
    Load_DB loader;
    Bank bank(loader);

    std::vector<int> accountIds;
    for (auto& [userId, user] : loader.getBankDb().getAll()) {
        for (auto& [accId, acc] : user->get_accounts().getAll()) {
            accountIds.push_back(accId);
        }
    }

    if (accountIds.size() < 2) {
        std::cerr << "Need at least 2 accounts loaded to run stress test\n";
        return 1;
    }
    std::cout << "Found " << accountIds.size() << " accounts to test against\n";

    //sum every account's initial balance
    long long totalBefore = 0;
    for (int id : accountIds) {
        long long bal;
        bank.getBalance(id, bal);
        totalBefore += bal;
    }
    std::cout << "Total balance before: " << totalBefore << " cents\n";

    JobHub hub;
    ResponseQueue responseQueue;
    WorkerPool pool(bank, loader, hub, responseQueue, 2, 8);
    //provided 2 login and 8 transaction threads

    const int NUM_PRODUCERS = 10; // 10 total threads
    const int REQUESTS_PER_PRODUCER = 200; //each thread processes 200 transactions each
    const int TOTAL_REQUESTS = NUM_PRODUCERS * REQUESTS_PER_PRODUCER;

    std::vector<pthread_t> producers(NUM_PRODUCERS);
    std::vector<ProducerArgs> argsList(NUM_PRODUCERS);

    std::cout << "\n Launching " << NUM_PRODUCERS << " producer threads, "
              << REQUESTS_PER_PRODUCER << " requests each ("
              << TOTAL_REQUESTS << " total) \n";

    for (int i = 0; i < NUM_PRODUCERS; i++) {
        argsList[i] = { &hub, &accountIds, REQUESTS_PER_PRODUCER };
        pthread_create(&producers[i], nullptr, producerFunc, &argsList[i]);
    }
    for (auto& t : producers) {
        pthread_join(t, nullptr);
    }
    std::cout << "All " << TOTAL_REQUESTS << " requests submitted\n";

    // Drain every response, accurately accumulating expected money movement.

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
            // TRANSFER contributes 0 as money just moves between two accounts
        } else {
            failCount++;
        }
    }

    std::cout << "\nProcessed " << (successCount + failCount) << " responses "
              << "(" << successCount << " succeeded, " << failCount << " failed/rejected)\n";

    //Sum every account's final balance, and simultaneously check none of them dipped below zero
    long long totalAfter = 0;
    bool anyNegative = false;
    for (int id : accountIds) {
        long long bal;
        bank.getBalance(id, bal);
        totalAfter += bal;
        if (bal < 0) {
            anyNegative = true;
            std::cerr << "NEGATIVE BALANCE detected on account " << id << ": " << bal << "\n";
        }
    }

    long long expectedTotal = totalBefore + expectedDelta;

    std::cout << "\nTotal balance before:  " << totalBefore << " cents\n";
    std::cout << "Expected delta:        " << expectedDelta << " cents\n";
    std::cout << "Expected total after:  " << expectedTotal << " cents\n";
    std::cout << "Actual total after:    " << totalAfter << " cents\n";

    bool moneyConserved = (totalAfter == expectedTotal);

    std::cout << "\n RESULT \n";
    if (moneyConserved && !anyNegative) {
        std::cout << "PASS: Money conserved exactly. No negative balances. No corruption under "
                   << NUM_PRODUCERS << " concurrent threads \n";
    } else {
        std::cout << "FAIL \n";
        if (!moneyConserved) std::cout << "  - Money NOT conserved (mismatch of "
                                        << (totalAfter - expectedTotal) << " cents)\n";
        if (anyNegative) std::cout << "  - Negative balance(s) detected\n";
    }

    return moneyConserved && !anyNegative ? 0 : 1;
}