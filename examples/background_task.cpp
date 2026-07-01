#include "olga_scheduler.hpp"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>

int main()
{
    using namespace std::chrono_literals;
    using Clock = std::chrono::steady_clock;

    olga_scheduler::EventLoop<Clock> loop;
    std::uint64_t                    counter    = 0;
    std::uint64_t                    bg_counter = 0;

    auto evt = loop.repeat(1s, [&](const auto& arg) {
        ++counter;
        std::cout << "counter=" << counter << " now=" << arg.approx_now.time_since_epoch().count() << '\n';
        if (counter > 10) {
            arg.event.cancel();
        }
    });

    auto run_background_step = [&] {
        ++bg_counter;
        std::cout << "bg_counter=" << bg_counter << '\n';
        std::this_thread::sleep_for(50ms);
    };

    while (!loop.isEmpty()) {
        const auto spin_result = loop.spin();

        if ((counter > 0) && !loop.isEmpty()) {
            for (int i = 0; (i < 3) && (Clock::now() < spin_result.next_deadline); ++i) {
                run_background_step();
            }
        }

        if (!loop.isEmpty()) {
            std::this_thread::sleep_until(spin_result.next_deadline);
        }
    }
    return 0;
}
