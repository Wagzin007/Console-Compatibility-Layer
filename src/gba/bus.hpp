#pragma once

#include "common/error.hpp"
#include "common/result.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace ccl::gba {

enum class AccessType : std::uint8_t {
    non_sequential,
    sequential,
};

enum class AccessWidth : std::uint8_t {
    byte = 1,
    halfword = 2,
    word = 4,
};

enum class GuestEventType : std::uint8_t {
    bios_unavailable,
    io_access,
    save_access,
    unmapped_access,
    unsupported_write,
};

enum class GuestAccess : std::uint8_t {
    read,
    write,
};

struct GuestEvent {
    GuestEventType type{};
    std::uint32_t address{};
    AccessWidth width{};
    GuestAccess access{};
};

struct BusAccessResult {
    std::uint32_t value{};
    std::uint32_t cycles{};
};

class GbaBus {
public:
    static constexpr std::size_t ewram_size = 0x40000;
    static constexpr std::size_t iwram_size = 0x8000;
    static constexpr std::size_t io_size = 0x400;
    static constexpr std::size_t palette_size = 0x400;
    static constexpr std::size_t vram_size = 0x18000;
    static constexpr std::size_t oam_size = 0x400;
    static constexpr std::size_t max_rom_size = 0x2000000;
    static constexpr std::size_t max_guest_events = 256;

    [[nodiscard]] static std::optional<Error> validate_rom_size(std::size_t size);
    [[nodiscard]] Result<std::size_t, Error> load_rom(std::span<const std::uint8_t> bytes);

    [[nodiscard]] BusAccessResult read8(std::uint32_t address, AccessType access = AccessType::non_sequential) {
        return read(address, AccessWidth::byte, access);
    }

    [[nodiscard]] BusAccessResult read16(std::uint32_t address, AccessType access = AccessType::non_sequential) {
        return read(address & ~std::uint32_t{1}, AccessWidth::halfword, access);
    }

    [[nodiscard]] BusAccessResult read32(std::uint32_t address, AccessType access = AccessType::non_sequential) {
        return read(address & ~std::uint32_t{3}, AccessWidth::word, access);
    }

    [[nodiscard]] std::uint32_t write8(
        std::uint32_t address,
        std::uint8_t value,
        AccessType access = AccessType::non_sequential) {
        return write(address, AccessWidth::byte, value, access);
    }

    [[nodiscard]] std::uint32_t write16(
        std::uint32_t address,
        std::uint16_t value,
        AccessType access = AccessType::non_sequential) {
        return write(address & ~std::uint32_t{1}, AccessWidth::halfword, value, access);
    }

    [[nodiscard]] std::uint32_t write32(
        std::uint32_t address,
        std::uint32_t value,
        AccessType access = AccessType::non_sequential) {
        return write(address & ~std::uint32_t{3}, AccessWidth::word, value, access);
    }

    // M2 does not track CPU fetches; this explicit fallback defaults to zero.
    void set_open_bus_value(std::uint32_t value) noexcept { open_bus_value_ = value; }
    [[nodiscard]] std::uint32_t open_bus_value() const noexcept { return open_bus_value_; }

    void set_strict_mode(bool strict) noexcept { strict_mode_ = strict; }
    [[nodiscard]] bool strict_mode() const noexcept { return strict_mode_; }

    [[nodiscard]] const std::vector<GuestEvent>& guest_events() const noexcept { return guest_events_; }
    [[nodiscard]] std::size_t guest_event_overflow_count() const noexcept { return guest_event_overflow_count_; }
    void clear_guest_events() noexcept {
        guest_events_.clear();
        guest_event_overflow_count_ = 0;
    }

private:
    [[nodiscard]] BusAccessResult read(std::uint32_t address, AccessWidth width, AccessType access) {
        const auto cycles = cycles_for(address, width, access);
        // GBATEK §§1.2.1-1.2.8: the high address byte selects the GBA bus region.
        switch (address >> 24U) {
        case 0x00:
            record_event(address <= 0x00003FFFU ? GuestEventType::bios_unavailable : GuestEventType::unmapped_access,
                address, width, GuestAccess::read);
            return {read_open_bus(address, width), cycles};
        case 0x02:
            return {read_le(ewram_, static_cast<std::size_t>(address & 0x00FFFFFFU) % ewram_size, width), cycles};
        case 0x03:
            return {read_le(iwram_, static_cast<std::size_t>(address & 0x00FFFFFFU) % iwram_size, width), cycles};
        case 0x04:
            return {read_io(address, width), cycles};
        case 0x05:
            return {read_le(palette_, static_cast<std::size_t>(address & 0x00FFFFFFU) % palette_size, width), cycles};
        case 0x06:
            return {read_le(vram_, mapped_vram_offset(address), width), cycles};
        case 0x07:
            return {read_le(oam_, static_cast<std::size_t>(address & 0x00FFFFFFU) % oam_size, width), cycles};
        case 0x08:
        case 0x09:
        case 0x0A:
        case 0x0B:
        case 0x0C:
        case 0x0D:
            return {read_rom(address, width), cycles};
        case 0x0E:
        case 0x0F:
            record_event(GuestEventType::save_access, address, width, GuestAccess::read);
            return {width == AccessWidth::byte ? 0xFFU
                : width == AccessWidth::halfword ? 0xFFFFU : 0xFFFFFFFFU, cycles};
        default:
            record_event(GuestEventType::unmapped_access, address, width, GuestAccess::read);
            return {read_open_bus(address, width), cycles};
        }
    }

    [[nodiscard]] std::uint32_t write(std::uint32_t address, AccessWidth width, std::uint32_t value, AccessType access) {
        const auto cycles = cycles_for(address, width, access);
        switch (address >> 24U) {
        case 0x00:
            record_event(address <= 0x00003FFFU ? GuestEventType::bios_unavailable : GuestEventType::unmapped_access,
                address, width, GuestAccess::write);
            break;
        case 0x02:
            write_le(ewram_, static_cast<std::size_t>(address & 0x00FFFFFFU) % ewram_size, width, value);
            break;
        case 0x03:
            write_le(iwram_, static_cast<std::size_t>(address & 0x00FFFFFFU) % iwram_size, width, value);
            break;
        case 0x04:
            write_io(address, width, value);
            break;
        case 0x05:
            write_palette(address, width, value);
            break;
        case 0x06:
            write_vram(address, width, value);
            break;
        case 0x07:
            if (width != AccessWidth::byte) {
                write_le(oam_, static_cast<std::size_t>(address & 0x00FFFFFFU) % oam_size, width, value);
            }
            break;
        case 0x08:
        case 0x09:
        case 0x0A:
        case 0x0B:
        case 0x0C:
        case 0x0D:
            record_event(GuestEventType::unsupported_write, address, width, GuestAccess::write);
            break;
        case 0x0E:
        case 0x0F:
            record_event(GuestEventType::save_access, address, width, GuestAccess::write);
            break;
        default:
            record_event(GuestEventType::unmapped_access, address, width, GuestAccess::write);
            break;
        }

        return cycles;
    }

    template <std::size_t Size>
    [[nodiscard]] static std::uint32_t read_le(
        const std::array<std::uint8_t, Size>& bytes, std::size_t offset, AccessWidth width) noexcept {
        std::uint32_t value = 0;
        for (std::uint32_t index = 0; index < static_cast<std::uint8_t>(width); ++index) {
            value |= static_cast<std::uint32_t>(bytes[offset + index]) << (index * 8U);
        }
        return value;
    }
    template <std::size_t Size>
    static void write_le(std::array<std::uint8_t, Size>& bytes, std::size_t offset, AccessWidth width,
        std::uint32_t value) noexcept {
        for (std::uint32_t index = 0; index < static_cast<std::uint8_t>(width); ++index) {
            bytes[offset + index] = static_cast<std::uint8_t>(value >> (index * 8U));
        }
    }
    [[nodiscard]] std::uint32_t read_io(std::uint32_t address, AccessWidth width);
    void write_io(std::uint32_t address, AccessWidth width, std::uint32_t value);
    void write_palette(std::uint32_t address, AccessWidth width, std::uint32_t value);
    void write_vram(std::uint32_t address, AccessWidth width, std::uint32_t value);
    [[nodiscard]] static std::size_t mapped_vram_offset(std::uint32_t address) noexcept;
    [[nodiscard]] std::uint32_t cycles_for(std::uint32_t address, AccessWidth width, AccessType access) const noexcept;
    [[nodiscard]] std::uint32_t read_rom(std::uint32_t address, AccessWidth width) const noexcept;
    [[nodiscard]] std::uint8_t read_rom_byte(std::uint32_t address) const noexcept;
    [[nodiscard]] std::uint32_t read_open_bus(std::uint32_t address, AccessWidth width) const noexcept;
    void record_event(GuestEventType type, std::uint32_t address, AccessWidth width, GuestAccess access);
    [[nodiscard]] std::uint32_t aligned_address(std::uint32_t address, AccessWidth width) const noexcept;

    std::array<std::uint8_t, ewram_size> ewram_{};
    std::array<std::uint8_t, iwram_size> iwram_{};
    std::array<std::uint8_t, io_size> io_{};
    std::array<std::uint8_t, palette_size> palette_{};
    std::array<std::uint8_t, vram_size> vram_{};
    std::array<std::uint8_t, oam_size> oam_{};
    std::vector<std::uint8_t> rom_;
    std::uint32_t open_bus_value_{};
    std::vector<GuestEvent> guest_events_;
    std::size_t guest_event_overflow_count_{};
    bool strict_mode_{};
};

} // namespace ccl::gba
