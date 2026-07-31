#include "database_loader.hpp"
#include "bank.hpp"
#include <iostream>

int main() {
    Load_DB loader;              // connects, loads everything — single-threaded, before anything else
    loader.display();             // sanity check: did real data come back from NeonDB?

    Bank bank(loader);            // builds the accountId -> userId secondary index

    long long balance;
    if (bank.getBalance(/* some real account_id from your seeded data */ 1, balance)) {
        std::cout << "Balance: " << balance << " cents\n";
    } else {
        std::cout << "Account not found\n";
    }

    return 0;
}