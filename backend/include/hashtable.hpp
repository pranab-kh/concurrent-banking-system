#ifndef HASHTABLE_HPP
#define HASHTABLE_HPP

#include <pthread.h>
#include <vector>
#include <list>
#include <utility>
#include <functional>
#include <stdexcept>
#include <atomic>


class MutexGuard {
public:
    explicit MutexGuard(pthread_mutex_t& m) : mutex_(m) {
        pthread_mutex_lock(&mutex_);
    }

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
    mutable std::vector<pthread_mutex_t> mutexes_; 
    std::atomic<size_t> entryCount_;   

public:
    explicit HashTable(size_t bucketCount = 16) : buckets_(bucketCount), mutexes_(bucketCount), entryCount_(0)
    {
        for (size_t i = 0; i < mutexes_.size(); ++i) 
        {
            if (pthread_mutex_init(&mutexes_[i], nullptr) != 0) 
            {
                throw std::runtime_error("Failed to initialize mutex");
            }
        }
    }

    ~HashTable()
    {
        for (size_t i = 0; i < mutexes_.size(); ++i) {
            pthread_mutex_destroy(&mutexes_[i]);
        }
    }

    HashTable(const HashTable&) = delete;
    HashTable& operator=(const HashTable&) = delete;

    void insert(const K& key, const V& value) 
    {
        size_t idx = bucketIndex(key);
        MutexGuard guard(mutexes_[idx]);
        // critical section
        Bucket& bucket = buckets_[idx];

        for (auto& entry : bucket) 
        {
            if (entry.first == key) {
                entry.second = value;
                return;
            }
        }

        bucket.push_back(std::make_pair(key, value));
        entryCount_++;
    }

    bool find(const K& key, V& val) const {
        size_t idx = bucketIndex(key);
        MutexGuard guard(mutexes_[idx]);

        //critical section
    
        const Bucket& bucket = buckets_[idx];
        for(const auto& entry: bucket){
            if(entry.first == key){
                val = entry.second;
                return true;
            }
        }
        return false;
    }

    bool remove(const K& key) {
        size_t idx = bucketIndex(key);
        MutexGuard guard(mutexes_[idx]);
        
        //critical section

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

    bool contains(const K& key) const 
    {
        V dummy;
        return find(key, dummy);
    }

    size_t size() const {
        return entryCount_.load();
    }   

private:
    size_t bucketIndex(const K& key) const {
        return std::hash<K>{}(key) % buckets_.size();
    }
};


#endif