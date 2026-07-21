#ifndef TRANSACTION_HPP
#define TRANSACTION_HPP

#include <ctime>
#include <optional>
#include <unordered_map>
#include<utility>
#include<cstdint>
#include<cstdlib>
#include<iomanip>

enum class TransactionType 
{
    DEPOSIT,
    WITHDRAWAL,
    TRANSFER_IN,
    TRANSFER_OUT
};

class Transaction {
private:
    int transaction_id;
    std::optional<int> from_account;
    std::optional<int> to_account;
    int64_t transaction_amount;
    std::string receiver_name;
    std::string receiver_mobile;
    std::string remarks;
    std::string transaction_status;
    std::string transaction_at;
    std::string transaction_type;

public:
    // Default constructor
    Transaction() = default;

    // // Delete copy operations (non-copyable)
    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;

    // // Default move operations (move-only)
    Transaction(Transaction&&) = default;
    Transaction& operator=(Transaction&&) = default;

    /**
     * Parameterized constructor using std::move for efficient data transfer.
     */
    Transaction(int transaction_id_,
                std::optional<int> from_account_,
                std::optional<int> to_account_,
                int64_t transaction_amount_,
                std::string receiver_name_,
                std::string receiver_mobile_,
                std::string remarks_,
                std::string transaction_status_,
                std::string transaction_at_,
                std::string transaction_type_)
        : transaction_id(std::move(transaction_id_)),
          from_account(std::move(from_account_)),
          to_account(std::move(to_account_)),
          transaction_amount(std::move(transaction_amount_)),
          receiver_name(std::move(receiver_name_)),
          receiver_mobile(std::move(receiver_mobile_)),
          remarks(std::move(remarks_)),
          transaction_status(std::move(transaction_status_)),
          transaction_at(std::move(transaction_at_)),
          transaction_type(std::move(transaction_type_)) {
    }
        // Getters
int get_transaction_id() const { return transaction_id; }
const std::optional<int>& get_from_account() const { return from_account; }
const std::optional<int>& get_to_account() const { return to_account; }
int64_t get_transaction_amount() const { return transaction_amount; }
const std::string& get_receiver_name() const { return receiver_name; }
const std::string& get_receiver_mobile() const { return receiver_mobile; }
const std::string& get_remarks() const { return remarks; }
const std::string& get_transaction_status() const { return transaction_status; }
const std::string& get_transaction_at() const { return transaction_at; }
const std::string& get_transaction_type() const { return transaction_type; }
};


#endif