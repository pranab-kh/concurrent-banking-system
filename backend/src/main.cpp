#include "database_loader.hpp"
#include "bank.hpp"
#include "worker_pool.hpp"
#include <iostream>
#include <unistd.h>

void printBalance(Bank& bank, int accountId, const std::string& label) {
    long long balance;
    if (bank.getBalance(accountId, balance)) {
        std::cout << "[" << label << "] Account " << accountId << " balance: " << balance << " cents\n";
    } else {
        std::cout << "[" << label << "] Account " << accountId << " NOT FOUND\n";
    }
}

int main() {
    std::cout << "Load DB\n";
    Load_DB loader;
    loader.display();

    std::cout << "Bank Lookup (In memory)\n";
    Bank bank(loader);

    int testAccountId = 2;
    printBalance(bank, testAccountId, "Before");

    std::cout << "Direct Bank::deposit\n";
    if (bank.deposit(testAccountId, 1000)) {
        std::cout << "Deposit succeeded\n";
    } else {
        std::cout << "Deposit FAILED\n";
    }
    printBalance(bank, testAccountId, "After direct deposit");

    std::cout << "WorkerPool and JobHub\n";
    JobHub hub;
    ResponseQueue responseQueue;
    WorkerPool pool(bank, loader, hub, responseQueue, 2, 4);

    TransactionRequest req;
    req.requestId = 1;
    req.type = TransactionType::DEPOSIT;
    req.accountId = testAccountId;
    req.amountCents = 500;
    hub.pushTransaction(req);

    sleep(1);   // give a worker thread time to process it

    TransactionResponse resp;
    if (responseQueue.pop(resp)) {
        std::cout << "WorkerPool response: " << resp.message
                   << ", new balance: " << resp.newBalanceCents << " cents\n";
    } else {
        std::cout << "No response received (unexpected)\n";
    }

    printBalance(bank, testAccountId, "After WorkerPool deposit");

    std::cout << "\Withdraw more than available\n";
    long long huge = 999999999999;
    if (!bank.withdraw(testAccountId, huge)) {
        std::cout << "Correctly rejected oversized withdrawal\n";
    } else {
        std::cout << "BUG: oversized withdrawal succeeded\n";
    }

    std::cout << "Transfer between two accounts\n";
    int accountA = 1, accountB = 2;   // check acc ids
    long long balA, balB;
    bank.getBalance(accountA, balA);
    bank.getBalance(accountB, balB);
    std::cout << "Before transfer -> A: " << balA << ", B: " << balB << "\n";

    if (bank.transfer(accountA, accountB, 200)) {
        std::cout << "Transfer SUCCESS\n";
    } else {
        std::cout << "Transfer FAILED\n";
    }

    bank.getBalance(accountA, balA);
    bank.getBalance(accountB, balB);
    std::cout << "After transfer  -> A: " << balA << ", B: " << balB << "\n";

    std::cout << "Success\n";
    return 0;
}