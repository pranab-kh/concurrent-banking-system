#include "bank.hpp"
#include <cstring>
#include <ctime>


Event::Event(EventType type, int threadId, int accountId, long value, const char *msg)
    : type(type), threadId(threadId), accountId(accountId), value(value)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    timestampUs = (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000ULL;

    message[0] = '\0';
    if (msg) {
        strncpy(message, msg, EVENT_MSG_LEN - 1);
        message[EVENT_MSG_LEN - 1] = '\0';
    }
}

EventQueue::EventQueue(int capacity)
    : buffer(new Event[capacity]),
      capacity(capacity),
      head(0),
      tail(0),
      count(0)
{
    pthread_mutex_init(&lock, nullptr);
}

EventQueue::~EventQueue() {
    pthread_mutex_destroy(&lock);
    delete[] buffer;
}

void EventQueue::push(const Event &ev) {
    pthread_mutex_lock(&lock);        

    if (count == capacity) {
        head = (head + 1) % capacity;
        count--;
    }

    buffer[tail] = ev;
    tail = (tail + 1) % capacity;
    count++;

    pthread_mutex_unlock(&lock);
}

bool EventQueue::tryPop(Event &out) {
    pthread_mutex_lock(&lock);

    if (count == 0) {
        pthread_mutex_unlock(&lock);
        return false;
    }

    out = buffer[head];
    head = (head + 1) % capacity;
    count--;

    pthread_mutex_unlock(&lock);
    return true;
}
