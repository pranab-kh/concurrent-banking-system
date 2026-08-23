cmake_minimum_required(VERSION 3.16)
project(ConcurrentBankingSystem CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBPQXX REQUIRED libpqxx)
find_package(Drogon REQUIRED)

include_directories(backend/include)

add_executable(bank_main
    backend/src/main.cpp
    backend/src/bcrypt.c
    backend/src/crypt_blowfish.c
    backend/src/crypt_gensalt.c
    backend/src/wrapper.c
)

add_executable(stress_test
    backend/src/stress_test.cpp
    backend/src/database_loader.cpp
    backend/src/bcrypt.c
    backend/src/crypt_blowfish.c
    backend/src/crypt_gensalt.c
    backend/src/wrapper.c
)

target_include_directories(stress_test PRIVATE ${LIBPQXX_INCLUDE_DIRS})
target_link_libraries(stress_test PRIVATE Threads::Threads ${LIBPQXX_LIBRARIES} pq Drogon::Drogon)
target_link_directories(stress_test PRIVATE ${LIBPQXX_LIBRARY_DIRS} /opt/homebrew/opt/libpq/lib)

target_include_directories(bank_main PRIVATE ${LIBPQXX_INCLUDE_DIRS})
target_link_libraries(bank_main PRIVATE Threads::Threads ${LIBPQXX_LIBRARIES} pq Drogon::Drogon)
target_link_directories(bank_main PRIVATE ${LIBPQXX_LIBRARY_DIRS} /opt/homebrew/opt/libpq/lib)

#add_executable(create_account_test
#   backend/src/create_account_test.cpp
#    backend/src/database_loader.cpp
#    backend/src/bcrypt.c
#    backend/src/crypt_blowfish.c
#    backend/src/crypt_gensalt.c
#   backend/src/wrapper.c
#)
#target_include_directories(create_account_test PRIVATE ${LIBPQXX_INCLUDE_DIRS})
#target_link_libraries(create_account_test PRIVATE Threads::Threads ${LIBPQXX_LIBRARIES} pq Drogon::Drogon)
#target_link_directories(create_account_test PRIVATE ${LIBPQXX_LIBRARY_DIRS} /opt/homebrew/opt/libpq/lib)

# Actual runnable server (starts Drogon, registers WebSocket controllers).
# Kept as a separate target/file so backend/src/main.cpp (the existing test
# harness) is untouched — see backend/src/server_main.cpp for details.
add_executable(bank_server
    backend/src/server_main.cpp
    backend/src/database_loader.cpp
    backend/src/bcrypt.c
    backend/src/crypt_blowfish.c
    backend/src/crypt_gensalt.c
    backend/src/wrapper.c
)
target_include_directories(bank_server PRIVATE ${LIBPQXX_INCLUDE_DIRS})
target_link_libraries(bank_server PRIVATE Threads::Threads ${LIBPQXX_LIBRARIES} pq Drogon::Drogon)
target_link_directories(bank_server PRIVATE ${LIBPQXX_LIBRARY_DIRS} /opt/homebrew/opt/libpq/lib)
