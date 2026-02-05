# olga_scheduler

[![Main Workflow](https://github.com/zubax/olga_scheduler/actions/workflows/main.yml/badge.svg)](https://github.com/zubax/olga_scheduler/actions/workflows/main.yml)

Generic single-file implementation of an EDF scheduler suitable for deeply embedded systems.
"OLGa" is a reference to the fact that it has a logarithmic asymptotic complexity.

**Simply copy `olga_scheduler.hpp` (C++) or `olga_scheduler.h` (C) into your project tree and you are ready to roll.**
The only dependency is the [CAVL header-only library](https://github.com/pavel-kirienko/cavl).

The usage instructions are provided in the comments.
The code is fully covered by manual tests with full state space exploration.

For development-related instructions please refer to the CI configuration files.
To release a new version, simply create a new tag.

<!--suppress CheckImageSize, HtmlDeprecatedAttribute -->
<p align="center">
    <img src="/docs/St_Olga_by_Nesterov_in_1892_(cropped).jpg" alt="Olga of Kiev" width=256>
</p>

## Examples

### C99

```c
#include "olga_scheduler.h"

#include <inttypes.h>
#include <stdio.h>
#include <time.h>

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
    olga_event_t evt     = OLGA_EVENT_INIT;
    olga_defer(&sched, get_microseconds(&sched) + 1000000, &counter, handler, &evt);

    for (;;) {
        olga_spin_result_t spin_result = olga_spin(&sched);
        (void) spin_result;  // Optional performance information here.
        usleep(1000);        // Do something else here: IO multiplexing, update scheduler stats, etc.
    }
    return 0;
}
```
