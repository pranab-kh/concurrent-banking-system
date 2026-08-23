// demonstrates the fraud detection:
// 1. loads the DB
// 2. deliberately creates a circular transfer pattern (A -> B -> C -> A)
// using the real Bank/Load_DB pipeline
// 3. reloads fresh, builds the graph, runs cycle detection
// 4. confirms the planted cycle is actually flagged

#include "database_loader.hpp"
#include "bank.hpp"
#include "fraud_detector.hpp"
#include <iostream>
#include <cstdlib>

int main() {
std::cout << "Step 1: Load DB \n";
Load_DB loader;
Bank bank(loader);

std::vector<int> accountIds;
for (auto& [userId, user] : loader.getBankDb().getAll()) {
for (auto& [accId, acc] : user->get_accounts().getAll()) {
accountIds.push_back(accId);
}
}

if (accountIds.size() < 3) {
std::cerr << "Need at least 3 accounts to demo a cycle\n";
return 1; //flag this in f-e
}

int A = 12, B = accountIds[1], C = accountIds[2];
std::cout << "\nStep 2: Planting a cycle A(" << A << ") -> B(" << B
<< ") -> C(" << C << ") -> A(" << A << ") \n";

const char* connStr = std::getenv("BANK_DB_URL");
pqxx::connection conn(connStr);

auto doTransfer = [&](int from, int to, long long amount) {
TransactionRequest req;
req.transaction_type = "TRANSFER";
req.account_id = from;
req.to_account = to;
req.transaction_amount = amount;
req.remarks = "fraud demo(cyclic)";
req.connection = nullptr;
bool ok = loader.transaction(req, bank, conn);
std::cout << " " << from << " -> " << to << ": " << (ok ? "OK" : "FAILED") << "\n";
};

doTransfer(A, B, 100000); // 1000.00
doTransfer(B, C, 98000); // ~2% fee, ratio 0.98 — within [0.95, 1.02]
doTransfer(C, A, 96500); // closes the loop

std::cout << "\nStep 3: Reload fresh, build graph, detect cycles \n";
Load_DB loaderFresh;

FraudDetector detector(10000); // setting limit for bare minimum to flag
detector.buildGraph(loaderFresh);
auto cycles = detector.detectCycles(loaderFresh);
detector.printReport(cycles);

return 0;
}