#include "common_engine.h"
#include <string.h>
#include <time.h>

Event make_event(EventType type, int thread_id, int account_id, long value, const char *message) {
    Event ev;
    ev.type = type;
    ev.thread_id = thread_id;
    ev.account_id = account_id;
    ev.value = value;

    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    ev.timestamp_us = (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000ULL;

    ev.message[0] = '\0';
    if (message) {
        strncpy(ev.message, message, EVENT_MSG_LEN - 1);
        ev.message[EVENT_MSG_LEN - 1] = '\0';
    }
    return ev;
}