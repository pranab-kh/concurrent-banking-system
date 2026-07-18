#include <iostream>
#include <pqxx/pqxx>
#include <cstdlib>
#include <string>
#include <cstdint>
#include <iomanip>

// Helper function to format raw BIGINT cents into a human-readable currency format (e.g., 500000 -> $5000.00)
void displayFormattedCurrency(const std::string& name, int64_t cents, const std::string& acc_type, const std::string& status) {
    long double dollars = static_cast<long double>(cents) / 100.00;
    
    std::cout << std::left << std::setw(12) << name 
              << std::setw(15) << acc_type
              << std::setw(12) << status
              << "$" << std::fixed << std::setprecision(2) << dollars << std::endl;
}

int main() {
    try {
        // 1. Pull the secure variable from your WSL Linux environment
        const char* db_env = std::getenv("BANK_DB_URL");
        if (!db_env) {
            std::cerr << "Error: BANK_DB_URL environment variable is missing!" << std::endl;
            return 1;
        }

        std::cout << "Connecting to Neon Cloud Database via WSL..." << std::endl;
        pqxx::connection conn(db_env);

        if (conn.is_open()) {
            std::cout << "Connection established successfully!\n" << std::endl;
            
            // 2. Open an isolation transaction block for reading
            pqxx::work tx{conn};
            
            // Query using a JOIN to fetch user names alongside their account properties
            std::string query = 
                "SELECT u.Full_Name, a.Account_Type, a.Account_Status, a.Actual_Balance "
                "FROM User_Table u "
                "JOIN Account_Table a ON u.User_Id = a.User_Id "
                "ORDER BY u.User_Id ASC;";
                
            pqxx::result res = tx.exec(query);
            tx.commit(); // Close transaction block cleanly

            // 3. Print Header formatting for the display table
            std::cout << "==========================================================" << std::endl;
            std::cout << std::left << std::setw(12) << "Account" 
                      << std::setw(15) << "Type" 
                      << std::setw(12) << "Status" 
                      << "Actual Balance" << std::endl;
            std::cout << "==========================================================" << std::endl;

            // 4. Iterate over the database result rows and map data to types
            for (const auto& row : res) {
                // Extract strings directly
                std::string account_holder = row["Full_Name"].as<std::string>();
                std::string account_type   = row["Account_Type"].as<std::string>();
                std::string account_status = row["Account_Status"].as<std::string>();
                
                // Extract the BIGINT column cleanly into a native C++ 64-bit integer (Cents)
                int64_t balance_cents      = row["Actual_Balance"].as<int64_t>();

                // Format and display the values row by row
                displayFormattedCurrency(account_holder, balance_cents, account_type, account_status);
            }
            std::cout << "==========================================================" << std::endl;
            
        } else {
            std::cerr << "Pipeline failed to open cleanly." << std::endl;
        }

    } catch (const std::exception &e) {
        std::cerr << "Runtime Exception Occurred: " << e.what() << std::endl;
    }
    return 0;
}
