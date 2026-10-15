#pragma once

#include <cstdint>
#include <optional>
#include <vector>

namespace ccl::gba {

enum class SchedulerEventType : std::uint8_t {
    scanline,
    timer,
    dma,
    interrupt,
};

struct SchedulerEvent {
    SchedulerEventType type{};
    std::uint32_t payload{};
};

using EventHandle = std::uint64_t;

struct DueEvent {
    std::uint64_t at{};
    EventHandle handle{};
    SchedulerEvent event{};
};

class Scheduler {
public:
    [[nodiscard]] EventHandle schedule(std::uint64_t at, SchedulerEvent event);
    [[nodiscard]] bool cancel(EventHandle handle);
    [[nodiscard]] std::optional<std::uint64_t> next_deadline() const noexcept;
    [[nodiscard]] std::vector<DueEvent> pop_due(std::uint64_t now);
    [[nodiscard]] std::uint64_t current_time() const noexcept { return current_time_; }

private:
    struct ScheduledEvent {
        std::uint64_t at{};
        EventHandle handle{};
        std::uint64_t sequence{};
        SchedulerEvent event{};
    };

    std::vector<ScheduledEvent> events_;
    std::uint64_t current_time_{};
    std::uint64_t next_sequence_{};
    EventHandle next_handle_{1};
};

} // namespace ccl::gba
