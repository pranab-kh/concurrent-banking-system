#ifndef PASSWORD_HASH_HPP
#define PASSWORD_HASH_HPP

#include <iostream>
#include <cstdlib>
#include "BCrypt.hpp"

inline bool hash_password(std::string &password)
{
    const char *pepper = std::getenv("PEPPER");

    if (!pepper)
    {
        std::cerr << "Environment Variable Not Set" << std::endl;
        return false;
    }

    password = password + std::string(pepper);
    
    password = BCrypt::generateHash(password, 12);
    return true;

   

    return true;
}
inline bool verify_password(std::string &password, std::string &hashed_password)
{
    const char *pepper = std::getenv("PEPPER");

    if (!pepper)
    {
        std::cerr << "Environment Variable Not Set" << std::endl;
        return false;
    }

    password = password + std::string(pepper);

     bool is_match = BCrypt::validatePassword(password, hashed_password);
    return is_match;
}

#endif