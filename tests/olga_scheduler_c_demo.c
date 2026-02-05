#define _POSIX_C_SOURCE 200809L

#include "olga_scheduler.h"

#include <inttypes.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

static int64_t get_microseconds(olga_t* sched)
{
    (void)sched;
    struct timespec ts;
    (void)clock_gettime(CLOCK_MONOTONIC, &ts);
    return ((int64_t)ts.tv_sec * 1000000) + (ts.tv_nsec / 1000);
}

static void handler(olga_t* sched, olga_event_t* event, int64_t now)
{
    uint64_t* counter = (uint64_t*)event->user;
    ++*counter;
    printf("counter=%" PRIu64 " now=%" PRId64 "\n", *counter, now);
    // Keep events triggering exactly at 1 Hz.
    olga_defer(sched, event->deadline + 1000000, event->user, handler, event);
}

int main(void)
{
    olga_t sched;
    olga_init(&sched, NULL, get_microseconds);

    uint64_t     counter = 0;
    olga_event_t event     = OLGA_EVENT_INIT;
    olga_defer(&sched, sched.now(&sched) + 1000000, &counter, handler, &event);

    for (;;) {
        olga_spin_result_t spin_result = olga_spin(&sched);
        (void)spin_result; // Optional performance information here.
        const struct timespec delay = { .tv_sec = 0, .tv_nsec = 1000 * 1000 };
        (void)nanosleep(&delay, NULL); // Do something else here: IO multiplexing, update scheduler stats, etc.
        if (counter > 10) {
            olga_cancel(&sched, &event);
            puts("Event canceled");
            break;
        }
    }
    return 0;
}
