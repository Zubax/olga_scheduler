/// Simple C99 demo/compile-check for olga_scheduler.h.

#include "olga_scheduler.h"

#include <stdint.h>

typedef struct
{
    int64_t now;
} DemoClock;

static int64_t demo_now(olga_t* const sched) { return ((DemoClock*)sched->user)->now; }

static void demo_handler(olga_t* const sched, olga_event_t* const event, const int64_t now)
{
    (void)sched;
    (void)event;
    (void)now;
}

int main(void)
{
    DemoClock clock = { 0 };
    olga_t    sched;
    olga_init(&sched, &clock, demo_now);

    olga_event_t event = OLGA_EVENT_INIT;
    olga_defer(&sched, 10, NULL, demo_handler, &event);

    const olga_spin_result_t out = olga_spin(&sched);
    (void)out;

    olga_cancel(&sched, &event);
    return 0;
}
