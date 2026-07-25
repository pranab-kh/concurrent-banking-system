cmake_minimum_required(VERSION 3.16)
project(ConcurrentBankingSystem CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBPQXX REQUIRED libpqxx)

include_directories(backend/include)

add_executable(bank_main backend/src/main.cpp)

target_include_directories(bank_main PRIVATE ${LIBPQXX_INCLUDE_DIRS})
target_link_libraries(bank_main PRIVATE Threads::Threads ${LIBPQXX_LIBRARIES} pq)
target_link_directories(bank_main PRIVATE ${LIBPQXX_LIBRARY_DIRS} /opt/homebrew/opt/libpq/lib)
