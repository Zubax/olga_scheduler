#include "olga_scheduler.hpp"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>

namespace {

constexpr auto          CounterInterval       = std::chrono::seconds{ 1 };
constexpr std::uint64_t CounterLimit          = 10;
constexpr auto          BackgroundTaskRuntime = std::chrono::milliseconds{ 50 };
constexpr int           BackgroundStepsPerRun = 3;

} // namespace

int main()
{
    using Clock = std::chrono::steady_clock;

    olga_scheduler::EventLoop<Clock> loop;
    std::uint64_t                    counter    = 0;
    std::uint64_t                    bg_counter = 0;

    auto evt = loop.repeat(CounterInterval, [&](const auto& arg) {
        ++counter;
        std::cout << "counter=" << counter << " now=" << arg.approx_now.time_since_epoch().count() << '\n';
        if (counter > CounterLimit) {
            arg.event.cancel();
        }
    });

    auto run_background_step = [&] {
        ++bg_counter;
        std::cout << "bg_counter=" << bg_counter << '\n';
        std::this_thread::sleep_for(BackgroundTaskRuntime);
    };

    while (!loop.isEmpty()) {
        const auto spin_result = loop.spin();

        if ((counter > 0) && !loop.isEmpty()) {
            for (int i = 0; (i < BackgroundStepsPerRun) && (Clock::now() < spin_result.next_deadline); ++i) {
                run_background_step();
            }
        }

        if (!loop.isEmpty()) {
            std::this_thread::sleep_until(spin_result.next_deadline);
        }
    }
    return 0;
}
