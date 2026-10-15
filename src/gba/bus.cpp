#include "gba/bus.hpp"

#include "gba/rom_header.hpp"

#include <algorithm>
#include <utility>

// GBA regions, mirrors, and access widths follow GBATEK §§1.2.1-1.2.8 and 1.16.6-1.16.7.
namespace ccl::gba {

Result<std::size_t, Error> GbaBus::load_rom(std::span<const std::uint8_t> bytes) {
    if (const auto error = validate_rom_size(bytes.size())) {
        return Result<std::size_t, Error>::failure(*error);
    }
    rom_.assign(bytes.begin(), bytes.end());
    return Result<std::size_t, Error>::success(bytes.size());
}

std::optional<Error> GbaBus::validate_rom_size(std::size_t size) {
    if (size > max_rom_size) {
        return Error{"ROM excede o limite de 32 MiB"};
    }
    return std::nullopt;
}

std::uint32_t GbaBus::read_io(std::uint32_t address, AccessWidth width) {
    const auto offset = static_cast<std::size_t>(address & 0x00FFFFFFU);
    if (offset + static_cast<std::uint8_t>(width) <= io_.size()) {
        record_event(GuestEventType::io_access, address, width, GuestAccess::read);
        std::uint32_t value = 0;
        for (std::uint32_t index = 0; index < static_cast<std::uint8_t>(width); ++index) {
            value |= static_cast<std::uint32_t>(io_[offset + index]) << (index * 8U);
        }
        return value;
    }
    record_event(GuestEventType::unmapped_access, address, width, GuestAccess::read);
    return read_open_bus(address, width);
}

void GbaBus::write_io(std::uint32_t address, AccessWidth width, std::uint32_t value) {
    const auto offset = static_cast<std::size_t>(address & 0x00FFFFFFU);
    if (offset + static_cast<std::uint8_t>(width) <= io_.size()) {
        record_event(GuestEventType::io_access, address, width, GuestAccess::write);
        for (std::uint32_t index = 0; index < static_cast<std::uint8_t>(width); ++index) {
            io_[offset + index] = static_cast<std::uint8_t>(value >> (index * 8U));
        }
    } else {
        record_event(GuestEventType::unmapped_access, address, width, GuestAccess::write);
    }
}

void GbaBus::write_palette(std::uint32_t address, AccessWidth width, std::uint32_t value) {
    const auto offset = static_cast<std::size_t>(address & 0x00FFFFFFU) % palette_size;
    if (width == AccessWidth::byte) {
        const auto aligned = offset & ~std::size_t{1};
        const auto byte = static_cast<std::uint8_t>(value);
        palette_[aligned] = byte;
        palette_[aligned + 1] = byte;
        return;
    }
    for (std::uint32_t index = 0; index < static_cast<std::uint8_t>(width); ++index) {
        palette_[offset + index] = static_cast<std::uint8_t>(value >> (index * 8U));
    }
}

void GbaBus::write_vram(std::uint32_t address, AccessWidth width, std::uint32_t value) {
    const auto offset = mapped_vram_offset(address);
    if (width == AccessWidth::byte) {
        const auto mode = io_[0] & 0x07U;
        const auto bg_obj_boundary = (mode >= 3 && mode <= 5) ? 0x14000U : 0x10000U;
        if (offset >= bg_obj_boundary) {
            return;
        }
        const auto aligned = offset & ~std::size_t{1};
        const auto byte = static_cast<std::uint8_t>(value);
        vram_[aligned] = byte;
        vram_[aligned + 1] = byte;
        return;
    }
    for (std::uint32_t index = 0; index < static_cast<std::uint8_t>(width); ++index) {
        vram_[offset + index] = static_cast<std::uint8_t>(value >> (index * 8U));
    }
}

std::size_t GbaBus::mapped_vram_offset(std::uint32_t address) noexcept {
    auto offset = static_cast<std::size_t>(address & 0x00FFFFFFU) % 0x20000U;
    if (offset >= 0x18000U) {
        offset -= 0x8000U;
    }
    return offset;
}

std::uint32_t GbaBus::cycles_for(std::uint32_t address, AccessWidth width, AccessType access) const noexcept {
    const auto region = address >> 24U;
    const bool word = width == AccessWidth::word;
    switch (region) {
    case 0x02:
        return word ? 6U : 3U;
    case 0x03:
    case 0x04:
    case 0x07:
    case 0x00:
        return 1U;
    case 0x05:
    case 0x06:
        return word ? 2U : 1U;
    case 0x08:
    case 0x09:
        // GBATEK §1.13.1: WAITCNT resets to zero. WS0 has N/S halfword costs 5/3.
        return access == AccessType::non_sequential ? (word ? 8U : 5U) : (word ? 6U : 3U);
    case 0x0A:
    case 0x0B:
        // Default WAIT1 second-access cost is 5 cycles, not WAIT0's 3.
        return access == AccessType::non_sequential ? (word ? 10U : 5U) : (word ? 10U : 5U);
    case 0x0C:
    case 0x0D:
        // Default WAIT2 second-access cost is 9 cycles; 32-bit accesses are two halfwords.
        return access == AccessType::non_sequential ? (word ? 14U : 5U) : (word ? 18U : 9U);
    case 0x0E:
    case 0x0F:
        return static_cast<std::uint8_t>(width) * 5U;
    default:
        return 1U;
    }
}

std::uint32_t GbaBus::read_rom(std::uint32_t address, AccessWidth width) const noexcept {
    std::uint32_t result = 0;
    for (std::uint32_t index = 0; index < static_cast<std::uint8_t>(width); ++index) {
        result |= static_cast<std::uint32_t>(read_rom_byte(address + index)) << (index * 8U);
    }
    return result;
}

std::uint8_t GbaBus::read_rom_byte(std::uint32_t address) const noexcept {
    const auto offset = static_cast<std::size_t>(address & 0x01FFFFFFU);
    if (offset < rom_.size()) {
        return rom_[offset];
    }
    // GBATEK §1.16.5 documents the no-cartridge ROM pattern as (address/2)&0xFFFF.
    // This M2 fallback applies that halfword pattern when an address exceeds the loaded ROM.
    const auto halfword = static_cast<std::uint16_t>((address >> 1U) & 0xFFFFU);
    return static_cast<std::uint8_t>(halfword >> ((address & 1U) * 8U));
}

std::uint32_t GbaBus::read_open_bus(std::uint32_t address, AccessWidth width) const noexcept {
    std::uint32_t result = 0;
    for (std::uint32_t index = 0; index < static_cast<std::uint8_t>(width); ++index) {
        const auto lane = (address + index) & 3U;
        const auto byte = static_cast<std::uint8_t>(open_bus_value_ >> (lane * 8U));
        result |= static_cast<std::uint32_t>(byte) << (index * 8U);
    }
    return result;
}

void GbaBus::record_event(GuestEventType type, std::uint32_t address, AccessWidth width, GuestAccess access) {
    const GuestEvent event{type, aligned_address(address, width), width, access};
    const auto found = std::find_if(guest_events_.begin(), guest_events_.end(), [&](const GuestEvent& existing) {
        return existing.type == event.type && existing.address == event.address;
    });
    if (found != guest_events_.end()) {
        return;
    }
    if (guest_events_.size() == max_guest_events) {
        ++guest_event_overflow_count_;
        return;
    }
    guest_events_.push_back(event);
}

std::uint32_t GbaBus::aligned_address(std::uint32_t address, AccessWidth width) const noexcept {
    switch (width) {
    case AccessWidth::halfword:
        return address & ~std::uint32_t{1};
    case AccessWidth::word:
        return address & ~std::uint32_t{3};
    case AccessWidth::byte:
        return address;
    }
    return address;
}

} // namespace ccl::gba
