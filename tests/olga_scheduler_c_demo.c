/// Simple C99 demo/compile-check for olga_scheduler.h.

#include "olga_scheduler.h"

#include <stdint.h>

typedef struct
{
    int64_t now;
} DemoClock;

static int64_t demo_now(void* const user) { return ((DemoClock*)user)->now; }

static void demo_handler(void* const user, const int64_t now)
{
    (void)user;
    (void)now;
}

int main(void)
{
    DemoClock clock = { 0 };
    olga_t    sched;
    olga_init(&sched, &clock, demo_now);

    olga_event_t event = { 0 };
    olga_defer(&sched, 10, NULL, demo_handler, &event);

    const olga_spin_result_t out = olga_spin(&sched);
    (void)out;

    olga_cancel(&sched, &event);
    return 0;
}
