#include "gba/scheduler.hpp"

#include <algorithm>
#include <cstdlib>

namespace ccl::gba {

EventHandle Scheduler::schedule(std::uint64_t at, SchedulerEvent event) {
    if (at < current_time_) {
        std::abort();
    }

    const ScheduledEvent scheduled{at, next_handle_++, next_sequence_++, event};
    const auto position = std::upper_bound(events_.begin(), events_.end(), scheduled,
        [](const ScheduledEvent& left, const ScheduledEvent& right) {
            return left.at < right.at || (left.at == right.at && left.sequence < right.sequence);
        });
    events_.insert(position, scheduled);
    return scheduled.handle;
}

bool Scheduler::cancel(EventHandle handle) {
    const auto found = std::find_if(events_.begin(), events_.end(), [handle](const ScheduledEvent& event) {
        return event.handle == handle;
    });
    if (found == events_.end()) {
        return false;
    }
    events_.erase(found);
    return true;
}

std::optional<std::uint64_t> Scheduler::next_deadline() const noexcept {
    if (events_.empty()) {
        return std::nullopt;
    }
    return events_.front().at;
}

std::vector<DueEvent> Scheduler::pop_due(std::uint64_t now) {
    if (now < current_time_) {
        std::abort();
    }
    current_time_ = now;

    std::vector<DueEvent> due;
    while (!events_.empty() && events_.front().at <= now) {
        const auto event = events_.front();
        events_.erase(events_.begin());
        due.push_back({event.at, event.handle, event.event});
    }
    return due;
}

} // namespace ccl::gba
