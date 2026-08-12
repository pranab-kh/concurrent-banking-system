#include "database_loader.hpp"
#include "bank.hpp"

void Load_DB::transaction(TransactionRequest &t, Bank& bank, pqxx::connection& conn)
{
    try
    {
        pqxx::work transactions(conn);

        if (t.transaction_type == "WITHDRAW")
        {
            std::string withdraw =
                "UPDATE Account_Table "
                "SET actual_balance = actual_balance - $1, "
                "    available_balance = available_balance - $1 "
                "WHERE account_id = $2 AND available_balance >= $1;";

            std::string transaction_log =
                "INSERT INTO Transaction_Table (account_id, transaction_type, transaction_amount, remarks) "
                "VALUES ($1, $2, $3, $4);";

            auto result = transactions.exec_params(withdraw, t.transaction_amount, t.account_id);
            if (result.affected_rows() == 0)
            {
                if (t.connection)
                    t.connection->send("Transaction Failed: Insufficient funds or invalid account");
                return;
            }

            transactions.exec_params(transaction_log, t.account_id, t.transaction_type, t.transaction_amount, t.remarks);
            transactions.commit();

            bank.applyMemoryUpdate(t);

            if (t.connection)
                t.connection->send("SUCCESS");
        }
        else if (t.transaction_type == "DEPOSIT")
        {
            std::string deposit =
                "UPDATE Account_Table "
                "SET actual_balance = actual_balance + $1, "
                "    available_balance = available_balance + $1 "
                "WHERE account_id = $2;";

            std::string transaction_log =
                "INSERT INTO Transaction_Table (account_id, transaction_type, transaction_amount, remarks) "
                "VALUES ($1, $2, $3, $4);";

            auto result = transactions.exec_params(deposit, t.transaction_amount, t.account_id);
            if (result.affected_rows() == 0)
            {
                if (t.connection)
                    t.connection->send("Transaction Failed: Account not found");
                return;
            }

            transactions.exec_params(transaction_log, t.account_id, t.transaction_type, t.transaction_amount, t.remarks);
            transactions.commit();

            bank.applyMemoryUpdate(t);

            if (t.connection)
                t.connection->send("SUCCESS");
        }
        else if (t.transaction_type == "TRANSFER")
        {
            std::string withdraw =
                "UPDATE Account_Table "
                "SET actual_balance = actual_balance - $1, "
                "    available_balance = available_balance - $1 "
                "WHERE account_id = $2 AND available_balance >= $1;";

            std::string deposit =
                "UPDATE Account_Table "
                "SET actual_balance = actual_balance + $1, "
                "    available_balance = available_balance + $1 "
                "WHERE account_id = $2;";

            std::string transaction_log_from =
                "INSERT INTO Transaction_Table (account_id, transaction_type, transaction_amount, remarks, to_account) "
                "VALUES ($1, $2, $3, $4, $5);";

            std::string transaction_log_to =
                "INSERT INTO Transaction_Table (account_id, transaction_type, transaction_amount, remarks, from_account) "
                "VALUES ($1, $2, $3, $4, $5);";

            auto result_1 = transactions.exec_params(withdraw, t.transaction_amount, t.account_id);
            if (result_1.affected_rows() == 0)
            {
                if (t.connection)
                    t.connection->send("Transfer Failed: Insufficient funds or sender missing");
                return;
            }

            auto result_2 = transactions.exec_params(deposit, t.transaction_amount, t.to_account);
            if (result_2.affected_rows() == 0)
            {
                if (t.connection)
                    t.connection->send("Transfer Failed: Recipient account missing");
                return;
            }

            transactions.exec_params(transaction_log_from, t.account_id, t.transaction_type, t.transaction_amount, t.remarks, t.to_account);
            transactions.exec_params(transaction_log_to, t.to_account, t.transaction_type, t.transaction_amount, t.remarks, t.account_id);

            transactions.commit();

            bank.applyMemoryUpdate(t);

            if (t.connection)
                t.connection->send("SUCCESS");
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "TRANSACTION SYSTEM ERROR: " << e.what() << std::endl;
        if (t.connection)
            t.connection->send("ERROR");
    }
}