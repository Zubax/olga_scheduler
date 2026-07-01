#include "olga_scheduler.hpp"

#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string_view>
#include <thread>

namespace {

using Clock = std::chrono::steady_clock;

constexpr std::uint64_t TaskRunCount   = 6;
constexpr auto          TaskPeriod     = std::chrono::milliseconds{ 200 };
constexpr auto          ShortWork      = std::chrono::milliseconds{ 25 };
constexpr auto          LongWork       = std::chrono::milliseconds{ 50 };
constexpr int           TaskColumn     = 14;
constexpr int           CountColumn    = 12;
constexpr int           DurationColumn = 14;

template <typename Duration>
auto toMicroseconds(const Duration duration)
{
    return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
}

template <typename Trace>
void printTrace(const Trace& trace)
{
    std::cout << "\ntask_trace\n";
    std::cout << std::left << std::setw(TaskColumn) << "task" << std::right << std::setw(CountColumn) << "executions"
              << std::setw(DurationColumn) << "total_us" << std::setw(DurationColumn) << "shortest_us"
              << std::setw(DurationColumn) << "longest_us" << std::setw(DurationColumn) << "average_us" << '\n';
    std::cout << std::string_view{ "----------------------------------------------"
                                   "--------------------------------------" }
              << '\n';
    for (const auto& task : trace) {
        const auto name = task.name.empty() ? std::string_view{ "<unnamed>" } : task.name;
        std::cout << std::left << std::setw(TaskColumn) << name << std::right << std::setw(CountColumn)
                  << task.execution_count << std::setw(DurationColumn) << toMicroseconds(task.total_duration)
                  << std::setw(DurationColumn) << toMicroseconds(task.shortest_duration) << std::setw(DurationColumn)
                  << toMicroseconds(task.longest_duration) << std::setw(DurationColumn)
                  << toMicroseconds(task.average_duration) << '\n';
    }
}

} // namespace

int main()
{
    olga_scheduler::EventLoop<Clock, true, 3U> loop;

    std::uint64_t sensor_cycles     = 0;
    std::uint64_t controller_cycles = 0;
    std::uint64_t estimator_cycles  = 0;

    const auto make_task = [](const std::string_view name, const Clock::duration work, std::uint64_t& cycles) {
        return [name, work, &cycles](const auto& arg) {
            ++cycles;
            std::cout << "task=" << name << " cycle=" << cycles << " now=" << arg.approx_now.time_since_epoch().count()
                      << '\n';
            std::this_thread::sleep_for(work);
            if (cycles >= TaskRunCount) {
                arg.event.cancel();
            }
        };
    };

    auto sensor_task     = loop.repeat("sensor", TaskPeriod, make_task("sensor", ShortWork, sensor_cycles));
    auto controller_task = loop.repeat("controller", TaskPeriod, make_task("controller", ShortWork, controller_cycles));
    auto estimator_task  = loop.repeat("estimator", TaskPeriod, make_task("estimator", LongWork, estimator_cycles));

    while (!loop.isEmpty()) {
        const auto spin_result = loop.spin();
        if (!loop.isEmpty()) {
            std::this_thread::sleep_until(spin_result.next_deadline);
        }
    }

    printTrace(loop.getTrace());
    return 0;
}
