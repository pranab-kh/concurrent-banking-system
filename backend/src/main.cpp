#include "database_loader.hpp"
#include "bank.hpp"
#include "worker_pool.hpp"
#include <iostream>
#include <unistd.h>

JobHub* globalJobHub = nullptr;

void printBalance(Bank& bank, int accountId, const std::string& label) {
    long long balance;
    if (bank.getBalance(accountId, balance)) {
        std::cout << "[" << label << "] Account " << accountId << " balance: " << balance << " cents\n";
    } else {
        std::cout << "[" << label << "] Account " << accountId << " NOT FOUND\n";
    }
}

int main() {
    std::cout << "Load DB \n";
    Load_DB loader;
    loader.display();

    std::cout << "Bank in-memory lookup\n";
    Bank bank(loader);

    int testAccountId = 2;  
    printBalance(bank, testAccountId, "Before");

    std::cout << "Bank::deposit\n";
    if (bank.deposit(testAccountId, 1000)) {
        std::cout << "Deposit succeeded\n";
    } else {
        std::cout << "Deposit FAILED\n";
    }
    printBalance(bank, testAccountId, "After direct deposit");

    std::cout << "WorkerPool + JobHub\n";
    JobHub hub;
    ResponseQueue responseQueue;
    WorkerPool pool(bank, loader, hub, responseQueue, 2, 4);
    globalJobHub = &hub;

    TransactionRequest req;
    req.account_id = testAccountId;
    req.transaction_type = "DEPOSIT";
    req.transaction_amount = 500;
    hub.pushTransaction(req);

    sleep(1);   // process time

    TransactionResponse resp;
    if (responseQueue.pop(resp)) {
        std::cout << "WorkerPool response: " << resp.message
                   << ", new balance: " << resp.newBalanceCents << " cents\n";
    } else {
        std::cout << "No response received (unexpected)\n";
    }

    printBalance(bank, testAccountId, "After WorkerPool deposit");

    std::cout << "Withdraw more than available (should fail) \n";
    long long huge = 999999999999;
    if (!bank.withdraw(testAccountId, huge)) {
        std::cout << "Correctly rejected oversized withdrawal\n";
    } else {
        std::cout << "BUG: oversized withdrawal succeeded\n";
    }

    std::cout << "Transfer between two accounts \n";
    int accountA = 2, accountB = 1;   
    long long balA, balB;
    bank.getBalance(accountA, balA);
    bank.getBalance(accountB, balB);
    std::cout << "Before transfer -> A: " << balA << ", B: " << balB << "\n";

    if (bank.transfer(accountA, accountB, 200)) {
        std::cout << "Transfer succeeded\n";
    } else {
        std::cout << "Transfer FAILED\n";
    }

    bank.getBalance(accountA, balA);
    bank.getBalance(accountB, balB);
    std::cout << "After transfer  -> A: " << balA << ", B: " << balB << "\n";

    std::cout << "Test completed!\n";
    return 0;
}