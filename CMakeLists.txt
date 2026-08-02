cmake_minimum_required(VERSION 3.10)
project(test_app)

set(CMAKE_CXX_STANDARD 17)

# Find Drogon
find_package(Drogon REQUIRED)

# Find Threads
find_package(Threads REQUIRED)

# Find libpqxx via pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBPQXX REQUIRED libpqxx)

# Add the executable
add_executable(test_app
    backend/src/test.cpp
    backend/src/bcrypt.c
    backend/src/crypt_blowfish.c
    backend/src/crypt_gensalt.c
    backend/src/wrapper.c
)

# Include directories
target_include_directories(test_app PRIVATE
    ${LIBPQXX_INCLUDE_DIRS}
    backend/include
)

# Link libraries
target_link_libraries(test_app
    PRIVATE
    Drogon::Drogon
    Threads::Threads
    ${LIBPQXX_LIBRARIES}
    pq
    jsoncpp
    argon2
)
