# Concurrent Banking System

A multi-user banking system built in C++20 to demonstrate correct behavior under real concurrency: many threads performing deposits, withdrawals, transfers, logins, and account creation simultaneously, against shared in-memory data structures and a live PostgreSQL database, without corruption, lost updates, or deadlock.

Built as a combined Data Structures & Algorithms / Operating Systems project — the codebase pairs a hand-built, generic, thread-safe hash table with the classic OS concurrency primitives (mutexes, condition variables, pthreads) and validates the result with a real, evidence-based concurrent stress test.


## Overview

The system supports:
- Account creation with a unique, fixed-length, randomly generated account number
- Login (bcrypt-hashed passwords, salted with a server-side pepper)
- Deposits, withdrawals, and transfers, processed **DB-first** (PostgreSQL is the single source of truth; in-memory state is a synchronized mirror)
- Concurrent processing via a fixed pool of worker threads, each with its own dedicated database connection
- A real, repeatable stress test that proves exact conservation of money across hundreds of concurrent, randomly generated operations

**Stack:** C++20 · POSIX Threads (pthreads) · CMake · PostgreSQL (via NeonDB) · libpqxx · bcrypt · Qt (frontend, in progress) · Drogon (planned REST/WebSocket layer)


## Building

Requires: CMake ≥ 3.16, a C++20 compiler, `libpqxx`, PostgreSQL client libraries, Drogon.

```bash
git clone <repo-url>
cd concurrent-banking-system

# macOS (Homebrew)
brew install cmake libpqxx drogon

mkdir cmake-build && cd cmake-build
cmake ..
cmake --build . --target stress_test
```

### Environment variables

Create a `.env` file at the project root (never committed — see `.env.example`):

```bash
export BANK_DB_URL="postgresql://user:password@host/dbname?sslmode=require"
export PEPPER="a-long-random-secret-string"
```

Load it before running any binary:
```bash
source .env
```

---

## Running

```bash
cd cmake-build
../scripts/run_stress_test.sh   # or, manually:
cd .. && source .env && cd cmake-build && ./stress_test
```

