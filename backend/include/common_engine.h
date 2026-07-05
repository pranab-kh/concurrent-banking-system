#ifndef COMMON_ENGINE_H
#define COMMON_ENGINE_H

#include<pthread.h>
#include<stdbool.h>
#include<stdint.h>

typedef enum{
    EV_THREAD_STARTED,
    EV_THREAD_RUNNING,
    EV_THREAD_WAITING,
    EV_THREAD_BLOCKED,
    EV_THREAD_TERMINATED,
    EV_ACCOUNT_LOCKED,
    EV_ACCOUNT_UNLOCKED,
    EV_LOG_MESSAGE
}EventType;

#define EVENT_MSG_LEN 128

typedef struct {
    EventType type;
    int thread_id;
    int account_id;
    long value;
    uint64_t timestamp_us;
    char message[EVENT_MSG_LEN];
}Event;

typedef struct{
    Event *buffer;
    int capacity;
    int head;
    int tail;
    int count;
    pthread_mutex_t lock;
    pthread_cond_t not_empty;
}EventQueue;

void event_queue_init(EventQueue *q, int capacity);
void event_queue_destroy(EventQueue *q);

void event_queue_push(EventQueue *q, Event ev);

bool event_queue_try_pop(EventQueue *q, Event *out);

Event make_event(EventType type, int thread_id, int account_id, long value, const char *message);

#endif
