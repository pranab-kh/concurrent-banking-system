#ifndef FRAUD_DETECTOR_HPP
#define FRAUD_DETECTOR_HPP

#include "database_loader.hpp"
#include "hashtable.hpp"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <ctime>
#include <cmath>
#include <algorithm>

struct Edge {
    int to;
    long long amount;
    time_t timestamp;   // parsed from the transaction's stored time string
};

class FraudDetector {
private:

    HashTable<int, std::vector<Edge>> graph_;

    double amountTolerance_;

    long long maxWindowSeconds_;

    long long minAmountThreshold_;   

    size_t maxCycleLength_;

    void addEdge(int from, int to, long long amount, time_t ts) {
        std::vector<Edge> edges;
        graph_.find(from, edges);
        edges.push_back({to, amount, ts});
        graph_.insert(from, edges);
    }

    static time_t parseTimestamp(const std::string& s) {
        struct tm tmVal = {};
        if (strptime(s.c_str(), "%Y-%m-%d %H:%M:%S", &tmVal) == nullptr) {
            return 0;
        }
        return timegm(&tmVal);
    }

    bool hopIsViable(const std::vector<Edge>& pathEdges, const Edge& e,
                      time_t earliest, time_t latest,
                      time_t& newEarliest, time_t& newLatest) const
    {
        if (e.amount < minAmountThreshold_) return false;

        if (!pathEdges.empty()) {
            long long prevAmount = pathEdges.back().amount;
            if (prevAmount <= 0) return false;
            double ratio = static_cast<double>(e.amount) / static_cast<double>(prevAmount);
            if (ratio < (1.0 - amountTolerance_) || ratio > 1.02) return false;
        }

        newEarliest = pathEdges.empty() ? e.timestamp : std::min(earliest, e.timestamp);
        newLatest = pathEdges.empty() ? e.timestamp : std::max(latest, e.timestamp);
        long long span = static_cast<long long>(newLatest - newEarliest);
        if (span < 0 || span > maxWindowSeconds_) return false;

        return true;
    }

    void findCyclesFrom(int startNode,
                         int node,
                         std::unordered_set<int>& onPath,
                         std::vector<int>& pathNodes,
                         std::vector<Edge>& pathEdges,
                         time_t earliest,
                         time_t latest,
                         std::vector<std::vector<int>>& flaggedCycles)
    {
        if (maxCycleLength_ != 0 && pathEdges.size() >= maxCycleLength_) {
            return;
        }

        std::vector<Edge> outgoing;
        if (!graph_.find(node, outgoing)) return;

        for (const Edge& e : outgoing) {
            if (e.to < startNode) continue;

            time_t newEarliest, newLatest;
            if (!hopIsViable(pathEdges, e, earliest, latest, newEarliest, newLatest)) {
                continue;
            }

            if (e.to == startNode) {
                std::vector<int> cycleNodes = pathNodes;
                cycleNodes.push_back(startNode);
                flaggedCycles.push_back(cycleNodes);
                continue;
            }

            if (onPath.count(e.to)) {
                continue;
            }

            onPath.insert(e.to);
            pathNodes.push_back(e.to);
            pathEdges.push_back(e);

            findCyclesFrom(startNode, e.to, onPath, pathNodes, pathEdges,
                           newEarliest, newLatest, flaggedCycles);

            pathEdges.pop_back();
            pathNodes.pop_back();
            onPath.erase(e.to);
        }
    }

public:
    explicit FraudDetector(long long minAmountThreshold = 10000,
                            double amountTolerance = 0.05,
                            long long maxWindowSeconds = 7 * 24 * 3600,
                            size_t maxCycleLength = 12)
        : amountTolerance_(amountTolerance), maxWindowSeconds_(maxWindowSeconds),
        minAmountThreshold_(minAmountThreshold), maxCycleLength_(maxCycleLength)
    {
    }

    void buildGraph(Load_DB& loader) {
        for (auto& [userId, user] : loader.getBankDb().getAll()) {
            for (auto& [accId, acc] : user->get_accounts().getAll()) {
                for (auto& [txnId, txn] : acc->get_transactions().getAll()) {
                    if (txn->get_transaction_type() == "TRANSFER" &&
                        txn->get_to_account().has_value())
                    {
                        time_t ts = parseTimestamp(txn->get_transaction_at());
                        addEdge(accId, txn->get_to_account().value(),
                                txn->get_transaction_amount(), ts);
                    }
                }
            }
        }
    }

    std::vector<std::vector<int>> detectCycles(Load_DB& loader) {
        std::vector<std::vector<int>> flagged;

        std::vector<int> allAccounts;
        for (auto& [userId, user] : loader.getBankDb().getAll()) {
            for (auto& [accId, acc] : user->get_accounts().getAll()) {
                allAccounts.push_back(accId);
            }
        }
        std::sort(allAccounts.begin(), allAccounts.end());

        for (int s : allAccounts) {
            std::unordered_set<int> onPath;
            std::vector<int> pathNodes;
            std::vector<Edge> pathEdges;

            onPath.insert(s);
            pathNodes.push_back(s);

            findCyclesFrom(s, s, onPath, pathNodes, pathEdges, 0, 0, flagged);
        }

        return flagged;
    }

    void printReport(const std::vector<std::vector<int>>& cycles) {
        std::cout << "\nMoney Laundering Cycle Detection Report \n";
        if (cycles.empty()) {
            std::cout << "No suspicious circular money-flow patterns detected.\n";
            return;
        }
        std::cout << "FLAGGED: " << cycles.size()
                  << " plausible round-tripping pattern(s) "
                  << "(amount-consistent, within time window):\n\n";
        for (size_t i = 0; i < cycles.size(); i++) {
            std::cout << "  Pattern " << (i + 1) << ": ";
            for (size_t j = 0; j < cycles[i].size(); j++) {
                std::cout << "Account " << cycles[i][j];
                if (j + 1 < cycles[i].size()) std::cout << " -> ";
            }
            std::cout << "\n";
        }
    }
};

#endif
