/// Source: https://github.com/zubax/olga_scheduler
///
/// A single-file header-only EDF scheduler for embedded applications.
///
/// Copyright (c) Zubax Robotics  <info@zubax.com>
///
/// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
/// documentation files (the "Software"), to deal in the Software without restriction, including without limitation
/// the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
/// and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
///
/// The above copyright notice and this permission notice shall be included in all copies or substantial portions of
/// the Software.
///
/// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
/// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
/// OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
/// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#pragma once

#include <cavl.h> // Add to your include paths: https://github.com/pavel-kirienko/cavl

#include <assert.h>
#include <stdint.h>

#ifdef __cplusplus
// This is, strictly speaking, useless because we do not define any functions with external linkage here,
// but it tells static analyzers that what follows should be interpreted as C code rather than C++.
extern "C"
{
#endif

/// Represents a user-handled future event.
/// When the handler is invoked, the even is already removed from the scheduler.
/// If necessary, it can be re-inserted immediately from within the handler with a new deadline.
/// The time units can be arbitrary.
typedef struct olga_event_t
{
    CAVL2_T base;
    int64_t deadline;
    void*   user;
    void (*handler)(void* user, int64_t now);
} olga_event_t;

/// The main scheduler type.
typedef struct olga_t
{
    CAVL2_T* events;
    void*    user;
    int64_t (*now)(void* user);
} olga_t;

/// Current state assessment returned from olga_spin().
typedef struct olga_spin_result_t
{
    int64_t next_deadline;
    int64_t worst_lateness;
    int64_t now;
} olga_spin_result_t;

/// To deinitialize, simply cancel all events; nothing else needs to be done.
/// The time units can be arbitrary.
static inline void olga_init(olga_t* const self, void* const user, int64_t (*const now)(void* user))
{
    assert(self != NULL);
    assert(now != NULL);
    // TODO implement
}

static inline CAVL2_RELATION olga_private_compare_deadline(const void* user, const CAVL2_T* event)
{
    const int64_t outer = *(const int64_t*)user;
    const int64_t inner = ((const olga_event_t*)event)->deadline;
    return (outer == inner) ? 0 : ((outer > inner) ? +1 : -1);
}

/// Schedule a one-time event.
/// The handler will be invoked at or asap after the deadline; the actual invocation time will be provided.
static inline void olga_defer(olga_t* const self,
                              const int64_t deadline,
                              void* const   user,
                              void (*const handler)(void* user, int64_t now),
                              olga_event_t* const out_event)
{
    assert(self != NULL);
    assert(handler != NULL);
    assert(out_event != NULL);
    // TODO schedule the event
}

/// No effect if the event has already been completed.
static inline void olga_cancel(olga_t* const self, olga_event_t* const event)
{
    assert(self != NULL);
    assert(event != NULL);
    // TODO use cavl2_remove_if().
}

/// Execute pending events strictly in the order of their deadlines until there are no pending events left.
/// Events with the same deadline are executed in the FIFO order.
/// This method should be invoked regularly to pump the event loop.
static inline olga_spin_result_t olga_spin(olga_t* const self)
{
    assert(self != NULL);
    // TODO spin until no ready events left, return the deadline of the soonest
}

#ifdef __cplusplus
}
#endif
