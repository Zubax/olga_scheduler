/// Source: https://github.com/zubax/olga_scheduler
///
/// Copyright (c) 2024 Zubax Robotics  <info@zubax.com>
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

#include "olga_scheduler.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)

namespace {

struct TestClock final
{
    int64_t now;
};

int64_t clock_now(olga_t* const sched) { return static_cast<TestClock*>(sched->user)->now; }

void clock_advance(TestClock* const clock, const int64_t delta) { clock->now += delta; }

struct CallLog final
{
    std::vector<int>     ids;
    std::vector<int64_t> times;
};

struct CallbackCtx final
{
    CallLog*   log;
    int        id;
    int64_t    expected_deadline;
    TestClock* clock;
    int64_t    advance_by;
};

void record_handler(olga_t* const sched, olga_event_t* const event, const int64_t now)
{
    (void)sched;
    auto* const ctx = static_cast<CallbackCtx*>(event->user);
    if (ctx->log != nullptr) {
        ctx->log->ids.push_back(ctx->id);
        ctx->log->times.push_back(now);
    }
    if (ctx->expected_deadline != INT64_MIN) {
        EXPECT_EQ(event->deadline, ctx->expected_deadline);
        EXPECT_GE(now, ctx->expected_deadline);
    }
    if ((ctx->advance_by != 0) && (ctx->clock != nullptr)) {
        ctx->clock->now += ctx->advance_by;
    }
}

} // namespace

TEST(OlgaSchedulerC, EmptySpin)
{
    TestClock clock{ .now = 10'000 };
    olga_t    sched;
    olga_init(&sched, &clock, clock_now);

    const olga_spin_result_t out = olga_spin(&sched);
    EXPECT_EQ(out.next_deadline, INT64_MAX);
    EXPECT_EQ(out.worst_lateness, 0);
    EXPECT_EQ(out.now, INT64_MIN);
}

TEST(OlgaSchedulerC, ComparatorOrdering)
{
    olga_event_t a{};
    olga_event_t b{};

    a.deadline = 10;
    a.seqno    = 1;
    b.deadline = 20;
    b.seqno    = 0;
    EXPECT_LT(olga_private_compare(&a, &b.base), 0);
    EXPECT_GT(olga_private_compare(&b, &a.base), 0);

    b.deadline = 10;
    b.seqno    = 2;
    EXPECT_LT(olga_private_compare(&a, &b.base), 0);

    b.seqno = 1;
    EXPECT_EQ(olga_private_compare(&a, &b.base), 0);
}

TEST(OlgaSchedulerC, BasicOrdering)
{
    TestClock clock{ .now = 10'000 };
    olga_t    sched;
    olga_init(&sched, &clock, clock_now);

    CallLog log{};

    CallbackCtx ctx_a{ .log = &log, .id = 1, .expected_deadline = 11'000, .clock = &clock, .advance_by = 0 };
    CallbackCtx ctx_b{ .log = &log, .id = 2, .expected_deadline = 10'100, .clock = &clock, .advance_by = 0 };
    CallbackCtx ctx_c{ .log = &log, .id = 3, .expected_deadline = 12'000, .clock = &clock, .advance_by = 0 };
    CallbackCtx ctx_d{ .log = &log, .id = 4, .expected_deadline = 12'000, .clock = &clock, .advance_by = 0 };

    olga_event_t evt_a = OLGA_EVENT_INIT;
    olga_event_t evt_b = OLGA_EVENT_INIT;
    olga_event_t evt_c = OLGA_EVENT_INIT;
    olga_event_t evt_d = OLGA_EVENT_INIT;

    olga_defer(&sched, ctx_a.expected_deadline, &ctx_a, record_handler, &evt_a);
    olga_defer(&sched, ctx_b.expected_deadline, &ctx_b, record_handler, &evt_b);
    olga_defer(&sched, ctx_c.expected_deadline, &ctx_c, record_handler, &evt_c);
    olga_defer(&sched, ctx_d.expected_deadline, &ctx_d, record_handler, &evt_d);

    olga_spin_result_t out = olga_spin(&sched);
    EXPECT_EQ(out.next_deadline, 10'100);
    EXPECT_EQ(out.worst_lateness, 0);
    EXPECT_EQ(out.now, 10'000);
    EXPECT_TRUE(log.ids.empty());

    clock_advance(&clock, 1'100);
    out = olga_spin(&sched);
    EXPECT_EQ(out.next_deadline, 12'000);
    EXPECT_EQ(out.worst_lateness, 1'000);
    EXPECT_EQ(out.now, 11'100);
    EXPECT_EQ(log.ids, (std::vector<int>{ 2, 1 }));
    EXPECT_EQ(log.times, (std::vector<int64_t>{ 11'100, 11'100 }));
    log.ids.clear();
    log.times.clear();

    clock_advance(&clock, 900);
    out = olga_spin(&sched);
    EXPECT_EQ(out.next_deadline, INT64_MAX);
    EXPECT_EQ(out.worst_lateness, 0);
    EXPECT_EQ(out.now, 12'000);
    EXPECT_EQ(log.ids, (std::vector<int>{ 3, 4 }));
    EXPECT_EQ(log.times, (std::vector<int64_t>{ 12'000, 12'000 }));
}

TEST(OlgaSchedulerC, FifoSameDeadline)
{
    TestClock clock{ .now = 0 };
    olga_t    sched;
    olga_init(&sched, &clock, clock_now);

    CallLog     log{};
    CallbackCtx ctx_a{ .log = &log, .id = 1, .expected_deadline = 1'000, .clock = &clock, .advance_by = 0 };
    CallbackCtx ctx_b{ .log = &log, .id = 2, .expected_deadline = 1'000, .clock = &clock, .advance_by = 0 };
    CallbackCtx ctx_c{ .log = &log, .id = 3, .expected_deadline = 1'000, .clock = &clock, .advance_by = 0 };

    olga_event_t evt_a = OLGA_EVENT_INIT;
    olga_event_t evt_b = OLGA_EVENT_INIT;
    olga_event_t evt_c = OLGA_EVENT_INIT;

    olga_defer(&sched, ctx_a.expected_deadline, &ctx_a, record_handler, &evt_a);
    olga_defer(&sched, ctx_b.expected_deadline, &ctx_b, record_handler, &evt_b);
    olga_defer(&sched, ctx_c.expected_deadline, &ctx_c, record_handler, &evt_c);

    clock.now                    = 1'000;
    const olga_spin_result_t out = olga_spin(&sched);
    EXPECT_EQ(out.next_deadline, INT64_MAX);
    EXPECT_EQ(out.worst_lateness, 0);
    EXPECT_EQ(out.now, 1'000);
    EXPECT_EQ(log.ids, (std::vector<int>{ 1, 2, 3 }));
}

TEST(OlgaSchedulerC, Cancel)
{
    TestClock clock{ .now = 0 };
    olga_t    sched;
    olga_init(&sched, &clock, clock_now);

    CallLog     log{};
    CallbackCtx ctx_a{ .log = &log, .id = 1, .expected_deadline = 100, .clock = &clock, .advance_by = 0 };
    CallbackCtx ctx_b{ .log = &log, .id = 2, .expected_deadline = 200, .clock = &clock, .advance_by = 0 };

    olga_event_t evt_a = OLGA_EVENT_INIT;
    olga_event_t evt_b = OLGA_EVENT_INIT;

    olga_defer(&sched, ctx_a.expected_deadline, &ctx_a, record_handler, &evt_a);
    olga_defer(&sched, ctx_b.expected_deadline, &ctx_b, record_handler, &evt_b);

    olga_cancel(&sched, &evt_a);

    clock.now                    = 200;
    const olga_spin_result_t out = olga_spin(&sched);
    EXPECT_EQ(out.next_deadline, INT64_MAX);
    EXPECT_EQ(out.worst_lateness, 0);
    EXPECT_EQ(out.now, 200);
    EXPECT_EQ(log.ids, (std::vector<int>{ 2 }));
}

TEST(OlgaSchedulerC, IsPending)
{
    TestClock clock{ .now = 0 };
    olga_t    sched;
    olga_init(&sched, &clock, clock_now);

    CallbackCtx  ctx{ .log = nullptr, .id = 0, .expected_deadline = INT64_MIN, .clock = &clock, .advance_by = 0 };
    olga_event_t evt = OLGA_EVENT_INIT;

    EXPECT_FALSE(olga_is_pending(&sched, &evt));

    olga_defer(&sched, 100, &ctx, record_handler, &evt);
    EXPECT_TRUE(olga_is_pending(&sched, &evt));

    olga_cancel(&sched, &evt);
    EXPECT_FALSE(olga_is_pending(&sched, &evt));

    olga_defer(&sched, 100, &ctx, record_handler, &evt);
    EXPECT_TRUE(olga_is_pending(&sched, &evt));

    clock.now = 100;
    (void)olga_spin(&sched);
    EXPECT_FALSE(olga_is_pending(&sched, &evt));
}

TEST(OlgaSchedulerC, OverdueSingle)
{
    TestClock clock{ .now = 0 };
    olga_t    sched;
    olga_init(&sched, &clock, clock_now);

    CallLog      log{};
    CallbackCtx  ctx{ .log = &log, .id = 1, .expected_deadline = 1'000, .clock = &clock, .advance_by = 0 };
    olga_event_t evt = OLGA_EVENT_INIT;

    olga_defer(&sched, ctx.expected_deadline, &ctx, record_handler, &evt);

    clock.now                    = 1'030;
    const olga_spin_result_t out = olga_spin(&sched);
    EXPECT_EQ(out.next_deadline, INT64_MAX);
    EXPECT_EQ(out.worst_lateness, 30);
    EXPECT_EQ(out.now, 1'030);
    EXPECT_EQ(log.ids, (std::vector<int>{ 1 }));
    EXPECT_EQ(log.times, (std::vector<int64_t>{ 1'030 }));
    EXPECT_EQ(evt.deadline, ctx.expected_deadline);
}

TEST(OlgaSchedulerC, LongRunningCallback)
{
    TestClock clock{ .now = 0 };
    olga_t    sched;
    olga_init(&sched, &clock, clock_now);

    CallLog     log{};
    CallbackCtx ctx_a{ .log = &log, .id = 1, .expected_deadline = 0, .clock = &clock, .advance_by = 100 };
    CallbackCtx ctx_b{ .log = &log, .id = 2, .expected_deadline = 20, .clock = &clock, .advance_by = 0 };

    olga_event_t evt_a = OLGA_EVENT_INIT;
    olga_event_t evt_b = OLGA_EVENT_INIT;

    olga_defer(&sched, ctx_a.expected_deadline, &ctx_a, record_handler, &evt_a);
    olga_defer(&sched, ctx_b.expected_deadline, &ctx_b, record_handler, &evt_b);

    const olga_spin_result_t out = olga_spin(&sched);
    EXPECT_EQ(out.next_deadline, INT64_MAX);
    EXPECT_EQ(out.worst_lateness, 80);
    EXPECT_EQ(out.now, 100);
    EXPECT_EQ(log.ids, (std::vector<int>{ 1, 2 }));
    EXPECT_EQ(log.times, (std::vector<int64_t>{ 0, 100 }));
}

TEST(OlgaSchedulerC, WorstLatenessKeepsMax)
{
    TestClock clock{ .now = 100 };
    olga_t    sched;
    olga_init(&sched, &clock, clock_now);

    CallLog     log{};
    CallbackCtx ctx_a{ .log = &log, .id = 1, .expected_deadline = 0, .clock = &clock, .advance_by = 0 };
    CallbackCtx ctx_b{ .log = &log, .id = 2, .expected_deadline = 60, .clock = &clock, .advance_by = 0 };

    olga_event_t evt_a{};
    olga_event_t evt_b{};

    olga_defer(&sched, ctx_a.expected_deadline, &ctx_a, record_handler, &evt_a);
    olga_defer(&sched, ctx_b.expected_deadline, &ctx_b, record_handler, &evt_b);

    const olga_spin_result_t out = olga_spin(&sched);
    EXPECT_EQ(out.next_deadline, INT64_MAX);
    EXPECT_EQ(out.worst_lateness, 100);
    EXPECT_EQ(out.now, 100);
    EXPECT_EQ(log.ids, (std::vector<int>{ 1, 2 }));
    EXPECT_EQ(log.times, (std::vector<int64_t>{ 100, 100 }));
}

TEST(OlgaSchedulerC, RescheduleAlreadyScheduled)
{
    TestClock clock{ .now = 0 };
    olga_t    sched;
    olga_init(&sched, &clock, clock_now);

    CallLog     log{};
    CallbackCtx ctx_a{ .log = &log, .id = 1, .expected_deadline = INT64_MIN, .clock = &clock, .advance_by = 0 };
    CallbackCtx ctx_b{ .log = &log, .id = 2, .expected_deadline = INT64_MIN, .clock = &clock, .advance_by = 0 };

    olga_event_t evt_a = OLGA_EVENT_INIT;
    olga_event_t evt_b = OLGA_EVENT_INIT;

    // Schedule event A at 100 and event B at 200
    olga_defer(&sched, 100, &ctx_a, record_handler, &evt_a);
    olga_defer(&sched, 200, &ctx_b, record_handler, &evt_b);

    // Reschedule event A to 300 (later than B)
    olga_defer(&sched, 300, &ctx_a, record_handler, &evt_a);

    // Now spin at time 250 - only B should fire (at 200)
    clock.now              = 250;
    olga_spin_result_t out = olga_spin(&sched);
    EXPECT_EQ(out.next_deadline, 300); // A is rescheduled to 300
    EXPECT_EQ(out.worst_lateness, 50);
    EXPECT_EQ(out.now, 250);
    EXPECT_EQ(log.ids, (std::vector<int>{ 2 })); // Only B fired
    EXPECT_EQ(log.times, (std::vector<int64_t>{ 250 }));
    log.ids.clear();
    log.times.clear();

    // Advance to 350 and spin - now A should fire
    clock.now = 350;
    out       = olga_spin(&sched);
    EXPECT_EQ(out.next_deadline, INT64_MAX);
    EXPECT_EQ(out.worst_lateness, 50);
    EXPECT_EQ(out.now, 350);
    EXPECT_EQ(log.ids, (std::vector<int>{ 1 })); // A fired
    EXPECT_EQ(log.times, (std::vector<int64_t>{ 350 }));
}

TEST(OlgaSchedulerC, RescheduleBeforeAndAfter)
{
    TestClock clock{ .now = 0 };
    olga_t    sched;
    olga_init(&sched, &clock, clock_now);

    CallLog     log{};
    CallbackCtx ctx{ .log = &log, .id = 1, .expected_deadline = INT64_MIN, .clock = &clock, .advance_by = 0 };

    olga_event_t evt = OLGA_EVENT_INIT;

    // Schedule event at 500
    olga_defer(&sched, 500, &ctx, record_handler, &evt);

    // Reschedule to earlier time (100)
    olga_defer(&sched, 100, &ctx, record_handler, &evt);

    // Spin at 150 - event should fire at 100
    clock.now                    = 150;
    const olga_spin_result_t out = olga_spin(&sched);
    EXPECT_EQ(out.next_deadline, INT64_MAX);
    EXPECT_EQ(out.worst_lateness, 50);
    EXPECT_EQ(out.now, 150);
    EXPECT_EQ(log.ids, (std::vector<int>{ 1 }));
    EXPECT_EQ(log.times, (std::vector<int64_t>{ 150 }));
}

TEST(OlgaSchedulerC, MultipleReschedules)
{
    TestClock clock{ .now = 0 };
    olga_t    sched;
    olga_init(&sched, &clock, clock_now);

    CallLog     log{};
    CallbackCtx ctx{ .log = &log, .id = 1, .expected_deadline = INT64_MIN, .clock = &clock, .advance_by = 0 };

    olga_event_t evt = OLGA_EVENT_INIT;

    // Schedule, reschedule multiple times
    olga_defer(&sched, 100, &ctx, record_handler, &evt);
    olga_defer(&sched, 200, &ctx, record_handler, &evt);
    olga_defer(&sched, 150, &ctx, record_handler, &evt);
    olga_defer(&sched, 300, &ctx, record_handler, &evt);

    // Spin at 350 - event should fire once at 300
    clock.now                    = 350;
    const olga_spin_result_t out = olga_spin(&sched);
    EXPECT_EQ(out.next_deadline, INT64_MAX);
    EXPECT_EQ(out.worst_lateness, 50);
    EXPECT_EQ(out.now, 350);
    EXPECT_EQ(log.ids, (std::vector<int>{ 1 })); // Fired once
    EXPECT_EQ(log.times, (std::vector<int64_t>{ 350 }));
}

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
