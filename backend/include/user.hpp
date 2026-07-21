#ifndef USER_HPP
#define USER_HPP

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <cmath>
#include "account.hpp"
#include "transaction.hpp"

class User
{

private:
    int user_id;
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
    std::unordered_map<int, Account> accounts;

public:
    User() = default;
    User(const User &) = delete;
    User &operator=(const User &) = delete;
    User(User &&) = default;
    User &operator=(User &&) = default;

    User(int user_id_,
         std::string full_name_,
         std::string address_,
         std::string mobile_,
         std::string email_,
         std::string gender_,
         std::string nid_,
         std::string password_hash_,
         std::string user_created_at_,
         std::string user_updated_at_,
         std::string login_status_,
         std::unordered_map<int, Account> accounts_)
        : user_id(std::move(user_id_)),
          full_name(std::move(full_name_)),
          address(std::move(address_)),
          mobile(std::move(mobile_)),
          email(std::move(email_)),
          gender(std::move(gender_)),
          nid(std::move(nid_)),
          password_hash(std::move(password_hash_)),
          user_created_at(std::move(user_created_at_)),
          user_updated_at(std::move(user_updated_at_)),
          login_status(std::move(login_status_)),
          accounts(std::move(accounts_))
    {
    }
    // Getters
int get_user_id() const { return user_id; }
const std::string& get_full_name() const { return full_name; }
const std::string& get_address() const { return address; }
const std::string& get_mobile() const { return mobile; }
const std::string& get_email() const { return email; }
const std::string& get_gender() const { return gender; }
const std::string& get_nid() const { return nid; }
const std::string& get_password_hash() const { return password_hash; }
const std::string& get_user_created_at() const { return user_created_at; }
const std::string& get_user_updated_at() const { return user_updated_at; }
const std::string& get_login_status() const { return login_status; }
const std::unordered_map<int, Account>& get_accounts() const { return accounts; }

};

#endif