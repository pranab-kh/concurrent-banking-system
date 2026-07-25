#include "bank.hpp"
#include "request_queue.hpp"
#include "worker_pool.hpp"
#include <iostream>
#include<unistd.h>

int main() {
    Bank bank;
    RequestQueue queue;
    WorkerPool pool(bank, queue, 4);

    bank.createAccount(1, "Pratik", 9999999);
    bank.createAccount(2, "Sakar", 9999999);

    TransactionRequest req;
    req.type = TransactionType::DEPOSIT;
    req.accountId = 1;
    req.amountCents = 500;
    queue.push(req);

    sleep(1);

    long long balance;
    if (bank.getBalance(1, balance)) {
        std::cout << "Account 1 balance:" << balance << " units\n";
    }

    return 0;
}