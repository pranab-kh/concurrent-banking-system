#ifndef ACCOUNT_NUMBER_GENERATOR_HPP
#define ACCOUNT_NUMBER_GENERATOR_HPP

#include <string>
#include <random>
#include <sstream>
#include <iomanip>

class AccountNumberGenerator {
public:
    // generares one random 12-digit numeric string (caller should verify via db insert)
    static std::string generateCandidate() {
        // thread_local: each thread gets its own random engine
        static thread_local std::random_device rd;
        static thread_local std::mt19937_64 gen(rd());

        std::uniform_int_distribution<long long> dist(100000000000LL, 999999999999LL);

        std::ostringstream oss;
        oss << dist(gen);
        return oss.str();
    }
};

#endif