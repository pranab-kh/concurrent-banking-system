#ifndef HASHTABLE_HPP
#define HASHTABLE_HPP

#include <pthread.h>
#include <vector>
#include <list>
#include <utility>
#include <functional>
#include<stdexcept>

template<typename K, typename V>
class HashTable{
private:
    using Bucket = std::list<std::pair<K,V>>;
    std::vector<Bucket> buckets_;
    size_t entryCount_;
    mutable pthread_mutex_t mutex_;

public:
    explicit HashTable(size_t bucketCount = 16)
        : buckets_(bucketCount), entryCount_(0)
    {
        if(pthread_mutex_init(&mutex_, nullptr) != 0){
            throw std::runtime_error("Failed to initialize mutex")
        }
    }

    ~HashTable() {
        pthread_mutex_destroy(&mutex_);
    }

    HashTable(const HashTable&) = delete;
    HashTable& operator=(const HashTable&) = delete;
};


#endif