#include "olga_scheduler.hpp"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>

namespace {

constexpr std::uint64_t CounterLimit = 10;

} // namespace

int main()
{
    using namespace std::chrono_literals;

    olga_scheduler::EventLoop<std::chrono::steady_clock> loop;
    std::uint64_t                                        counter = 0;

    auto evt = loop.repeat(1s, [&](const auto& arg) {
        ++counter;
        std::cout << "counter=" << counter << " now=" << arg.approx_now.time_since_epoch().count() << '\n';
        if (counter > CounterLimit) {
            arg.event.cancel();
        }
    });

    while (!loop.isEmpty()) {
        (void)loop.spin();
        std::this_thread::sleep_for(1ms); // Do something else here.
    }
    return 0;
}
