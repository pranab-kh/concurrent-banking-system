#include <iostream>
#include <string>
#include<unordered_map>>
#include <vector>
#include <cmath>
#include "account.hpp"
#include "transaction.hpp"

class User
{
    public:
    int account_id;
    std::string user_id;
    std::string full_name;
    std::string address;
    std::string mobile;
    std::string email;
    std::string gender;
    std::string nid;
    std::string password_hash;
    std::string user_created_at;
    std::string user_updated_at;
    std::string login_status;
    std::unordered_map<int,Account>  accounts;



};