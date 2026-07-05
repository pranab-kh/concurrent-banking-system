#ifndef COMMON_ENGINE_HPP
#define COMMON_ENGINE_HPP

#include<pthread.h>
#include<cstdint>

enum class EventType {
    ThreadStarted,
    ThreadRunning,
    ThreadWaiting,
    ThreadBlocked,
    ThreadTerminated,
    AccountLocked,
    AccountUnlocked,
    LogMessage
};

const int EVENT_MSG_LEN = 128;

struct Event {
    EventType type;
    int threadId;
    int accountId;
    long value;
    uint64_t timestampUs;
    char message[EVENT_MSG_LEN];

    Event(EventType type, int threadId, int accountId, long value, const char* msg);
    Event() = default;   // needed bcs EventQueue can allocate an array of empty events
};

class EventQueue {
public:
    explicit EventQueue(int capacity);
    ~EventQueue();

    EventQueue(const EventQueue&) = delete;
    EventQueue& operator=(const EventQueue&) = delete;

    void push(const Event& ev);
    bool tryPop(Event& out);

private:
    Event* buffer;
    int capacity;
    int head;
    int tail;
    int count;
    pthread_mutex_t lock;
};

#endif
