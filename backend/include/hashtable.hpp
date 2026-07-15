#ifndef HASHTABLE_HPP
#define HASHTABLE_HPP

#include <pthread.h>
#include <vector>
#include <list>
#include <utility>
#include <functional>
#include<stdexcept>

class MutexGuard {
public:
    explicit MutexGuard(pthread_mutex_t& m) : mutex_(m) {
        pthread_mutex_lock(&mutex_);
    }

    //sensitive content

    ~MutexGuard() {
        pthread_mutex_unlock(&mutex_);
    }
    MutexGuard(const MutexGuard&) = delete;
    MutexGuard& operator=(const MutexGuard&) = delete;

private:
    pthread_mutex_t& mutex_;
};

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

    void insert(const K& key, const V& value) {
    MutexGuard guard(mutex_);

    size_t idx = bucketIndex(key);
    Bucket& bucket = buckets_[idx];

    for (auto& entry : bucket) {
        if (entry.first == key) {
            entry.second = value;
            return;
        }
    }

    bucket.push_back(std::make_pair(key, value));
    entryCount_++;
}

    bool find(const K& key, V& val) const {
        MutexGuard guard(mutex_);
        size_t indx = bucketIndex(key);
        const Bucket& Bucket = buckets_[indx];
        for(const auto& entry: bucket){
            if(entry.first == key){
                val = entry.second;
                return true;
            }
        return False;
        }
    }

    bool remove(const K& key) {
        MutexGuard guard(mutex_);

        size_t idx = bucketIndex(key);
        Bucket& bucket = buckets_[idx];

        for (auto it = bucket.begin(); it != bucket.end(); ++it) {
            if (it->first == key) {
                bucket.erase(it);
                entryCount_--;
                return true;
            }
        }

        return false;
    }

private:
    size_t bucketIndex(const K& key) const {
        return std::hash<K>{}(key) % buckets_.size();
    }
};



#endif