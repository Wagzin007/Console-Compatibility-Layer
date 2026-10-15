#include <doctest/doctest.h>

#include "gba/bus.hpp"
#include "gba/rom_header.hpp"
#include "gba/scheduler.hpp"
#include "gba/timing.hpp"

#include <array>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

#include <csignal>
#include <cstdio>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {

using ccl::gba::AccessType;
using ccl::gba::GbaBus;

std::vector<std::uint8_t> small_rom() {
    return {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
}

} // namespace

TEST_CASE("as constantes de tempo correspondem a linhas e frames GBA") {
    CHECK(ccl::gba::cycles_per_scanline == 1232);
    CHECK(ccl::gba::scanlines_per_frame == 228);
    CHECK(ccl::gba::cycles_per_frame == 280896);
}

TEST_CASE("GbaBus armazena dados little-endian e força alinhamento") {
    GbaBus bus;
    CHECK(bus.write32(0x02000003, 0x12345678) == 6);
    CHECK(bus.read8(0x02000000).value == 0x78);
    CHECK(bus.read8(0x02000001).value == 0x56);
    CHECK(bus.read16(0x02000001).value == 0x5678);
    CHECK(bus.read32(0x02000003).value == 0x12345678);
}

TEST_CASE("EWRAM, IWRAM, paleta, VRAM e OAM espelham seus limites") {
    GbaBus bus;

    static_cast<void>(bus.write32(0x0203FFFC, 0xA1B2C3D4));
    CHECK(bus.read32(0x0207FFFC).value == 0xA1B2C3D4);
    static_cast<void>(bus.write8(0x02000000, 0x5A));
    CHECK(bus.read8(0x02040000).value == 0x5A);

    static_cast<void>(bus.write32(0x03007FFC, 0x10203040));
    CHECK(bus.read32(0x0300FFFC).value == 0x10203040);
    static_cast<void>(bus.write8(0x03000000, 0x6B));
    CHECK(bus.read8(0x03008000).value == 0x6B);

    static_cast<void>(bus.write32(0x050003FC, 0x50607080));
    CHECK(bus.read32(0x050007FC).value == 0x50607080);

    static_cast<void>(bus.write32(0x06017FFC, 0x90ABCDEF));
    CHECK(bus.read32(0x06017FFC).value == 0x90ABCDEF);
    static_cast<void>(bus.write32(0x06010000, 0x11223344));
    CHECK(bus.read32(0x06018000).value == 0x11223344);
    CHECK(bus.read32(0x06030000).value == 0x11223344);

    static_cast<void>(bus.write32(0x070003FC, 0xCAFEBABE));
    CHECK(bus.read32(0x070007FC).value == 0xCAFEBABE);
}

TEST_CASE("ROM tem os mesmos bytes nas três janelas e rejeita ROM acima do limite") {
    GbaBus bus;
    auto source = small_rom();
    const auto loaded = bus.load_rom(source);
    REQUIRE(loaded.has_value());
    CHECK(loaded.value() == source.size());
    const auto source_before = source;
    source[0] = 0xFF;

    for (const std::uint32_t base : {0x08000000U, 0x0A000000U, 0x0C000000U}) {
        CHECK(bus.read8(base).value == 0x11);
        CHECK(bus.read8(base + 7).value == 0x88);
        CHECK(bus.read16(base).value == 0x2211);
        CHECK(bus.read32(base + 4).value == 0x88776655);
    }
    CHECK(bus.read8(0x09FFFFFF).value == 0xFF);
    CHECK(bus.read8(0x0BFFFFFF).value == 0xFF);
    CHECK(bus.read8(0x0DFFFFFF).value == 0xFF);
    CHECK(source_before[0] == 0x11);
    CHECK(source[0] == 0xFF);
    CHECK_FALSE(GbaBus::validate_rom_size(GbaBus::max_rom_size).has_value());
    const auto size_error = GbaBus::validate_rom_size(GbaBus::max_rom_size + 1);
    REQUIRE(size_error.has_value());
    CHECK(size_error->message.find("32 MiB") != std::string::npos);
}

TEST_CASE("acessos a registradores de I/O preservam bytes e emitem evento deduplicado") {
    GbaBus bus;
    bus.set_strict_mode(true);
    CHECK(bus.strict_mode());
    CHECK(bus.write16(0x040003FF, 0xBEEF) == 1);
    CHECK(bus.read16(0x040003FE).value == 0xBEEF);
    REQUIRE(bus.guest_events().size() == 1);
    CHECK(bus.guest_events()[0].type == ccl::gba::GuestEventType::io_access);
    CHECK(bus.guest_events()[0].address == 0x040003FE);
    CHECK(bus.guest_events()[0].width == ccl::gba::AccessWidth::halfword);
    CHECK(bus.guest_events()[0].access == ccl::gba::GuestAccess::write);
    CHECK(bus.strict_mode());
    CHECK_FALSE(bus.guest_events().empty());
}

TEST_CASE("escrita de byte na paleta e em BG VRAM duplica os dois bytes") {
    GbaBus bus;
    static_cast<void>(bus.write8(0x05000001, 0xA5));
    CHECK(bus.read16(0x05000000).value == 0xA5A5);

    static_cast<void>(bus.write8(0x06000001, 0x3C));
    CHECK(bus.read16(0x06000000).value == 0x3C3C);
}

TEST_CASE("escrita de byte em VRAM respeita a área BG/OBJ dos modos 0 a 5") {
    GbaBus bus;
    for (const std::uint16_t mode : {0, 2}) {
        static_cast<void>(bus.write16(0x04000000, mode));
        static_cast<void>(bus.write16(0x06010000, 0x1357));
        static_cast<void>(bus.write8(0x06010000, 0xAA));
        CHECK(bus.read16(0x06010000).value == 0x1357);
        static_cast<void>(bus.write8(0x06000000, 0x5A));
        CHECK(bus.read16(0x06000000).value == 0x5A5A);
    }
    for (const std::uint16_t mode : {3, 5}) {
        static_cast<void>(bus.write16(0x04000000, mode));
        static_cast<void>(bus.write16(0x06014000, 0x2468));
        static_cast<void>(bus.write8(0x06014000, 0xCC));
        CHECK(bus.read16(0x06014000).value == 0x2468);
        static_cast<void>(bus.write8(0x06010000, 0x69));
        CHECK(bus.read16(0x06010000).value == 0x6969);
    }
}

TEST_CASE("escrita de byte em OAM é ignorada") {
    GbaBus bus;
    static_cast<void>(bus.write16(0x07000000, 0xCAFE));
    static_cast<void>(bus.write8(0x07000001, 0x12));
    CHECK(bus.read16(0x07000000).value == 0xCAFE);
}

TEST_CASE("ROM além do fim usa o padrão de halfword baseado no endereço") {
    GbaBus bus;
    const std::array<std::uint8_t, 2> bytes{0xAA, 0xBB};
    REQUIRE(bus.load_rom(bytes).has_value());
    CHECK(bus.read8(0x08000002).value == 0x01);
    CHECK(bus.read8(0x08000003).value == 0x00);
    CHECK(bus.read16(0x08000002).value == 0x0001);
    CHECK(bus.read32(0x08000004).value == 0x00030002);
}

TEST_CASE("BIOS ausente, SRAM stub e regiões não mapeadas geram eventos") {
    GbaBus bus;
    bus.set_open_bus_value(0x12345678);
    CHECK(bus.read8(0x00000000).value == 0x78);
    CHECK(bus.read16(0x00003FFF).value == 0x1234);
    CHECK(bus.read8(0x00004000).value == 0x78);
    CHECK(bus.read8(0x0E000000).value == 0xFF);
    CHECK(bus.read16(0x0F000000).value == 0xFFFF);
    CHECK(bus.write8(0x0E000000, 0x34) == 5);
    CHECK(bus.read32(0x10000000).value == 0x12345678);
    CHECK(bus.read32(0xFFFFFFFC).value == 0x12345678);
    CHECK(bus.read8(0x0FFFFFFF).value == 0xFF);

    bool saw_bios = false;
    bool saw_unmapped = false;
    bool saw_save = false;
    for (const auto& event : bus.guest_events()) {
        saw_bios |= event.type == ccl::gba::GuestEventType::bios_unavailable;
        saw_unmapped |= event.type == ccl::gba::GuestEventType::unmapped_access;
        saw_save |= event.type == ccl::gba::GuestEventType::save_access;
    }
    CHECK(saw_bios);
    CHECK(saw_unmapped);
    CHECK(saw_save);
}

TEST_CASE("eventos têm limite, deduplicação e contador de overflow") {
    GbaBus bus;
    bus.set_strict_mode(true);
    static_cast<void>(bus.write8(0x04000010, 1));
    static_cast<void>(bus.read8(0x04000010));
    CHECK(bus.guest_events().size() == 1);
    CHECK(bus.guest_event_overflow_count() == 0);

    bus.clear_guest_events();
    for (std::uint32_t offset = 0; offset <= GbaBus::max_guest_events; ++offset) {
        static_cast<void>(bus.read8(0x04000000 + offset));
    }
    CHECK(bus.guest_events().size() == GbaBus::max_guest_events);
    CHECK(bus.guest_event_overflow_count() == 1);
}

TEST_CASE("ciclos por largura, região, janela ROM e tipo sequencial são explícitos") {
    GbaBus bus;
    CHECK(bus.read8(0x02000000).cycles == 3);
    CHECK(bus.read16(0x02000000).cycles == 3);
    CHECK(bus.read32(0x02000000).cycles == 6);
    CHECK(bus.read8(0x03000000).cycles == 1);
    CHECK(bus.read16(0x03000000).cycles == 1);
    CHECK(bus.read32(0x03000000).cycles == 1);
    CHECK(bus.read8(0x04000000).cycles == 1);
    CHECK(bus.read16(0x04000000).cycles == 1);
    CHECK(bus.read32(0x04000000).cycles == 1);
    CHECK(bus.read8(0x05000000).cycles == 1);
    CHECK(bus.read16(0x05000000).cycles == 1);
    CHECK(bus.read32(0x05000000).cycles == 2);
    CHECK(bus.read8(0x06000000).cycles == 1);
    CHECK(bus.read16(0x06000000).cycles == 1);
    CHECK(bus.read32(0x06000000).cycles == 2);
    CHECK(bus.read8(0x07000000).cycles == 1);
    CHECK(bus.read16(0x07000000).cycles == 1);
    CHECK(bus.read32(0x07000000).cycles == 1);

    CHECK(bus.read8(0x08000000, AccessType::non_sequential).cycles == 5);
    CHECK(bus.read8(0x08000000, AccessType::sequential).cycles == 3);
    CHECK(bus.read16(0x08000000, AccessType::non_sequential).cycles == 5);
    CHECK(bus.read16(0x08000000, AccessType::sequential).cycles == 3);
    CHECK(bus.read32(0x08000000, AccessType::non_sequential).cycles == 8);
    CHECK(bus.read32(0x08000000, AccessType::sequential).cycles == 6);
    CHECK(bus.read16(0x0A000000, AccessType::non_sequential).cycles == 5);
    CHECK(bus.read16(0x0A000000, AccessType::sequential).cycles == 5);
    CHECK(bus.read32(0x0A000000, AccessType::sequential).cycles == 10);
    CHECK(bus.read32(0x0A000000, AccessType::non_sequential).cycles == 10);
    CHECK(bus.read8(0x0C000000, AccessType::non_sequential).cycles == 5);
    CHECK(bus.read8(0x0C000000, AccessType::sequential).cycles == 9);
    CHECK(bus.read16(0x0C000000, AccessType::non_sequential).cycles == 5);
    CHECK(bus.read16(0x0C000000, AccessType::sequential).cycles == 9);
    CHECK(bus.read32(0x0C000000, AccessType::non_sequential).cycles == 14);
    CHECK(bus.read32(0x0C000000, AccessType::sequential).cycles == 18);
    CHECK(bus.read8(0x0E000000).cycles == 5);
    CHECK(bus.read16(0x0E000000).cycles == 10);
    CHECK(bus.read32(0x0E000000).cycles == 20);
}

TEST_CASE("fuzz determinístico de leituras e escritas do bus não causa crash") {
    GbaBus bus;
    std::mt19937 random{0xB05B05U};
    std::uniform_int_distribution<std::uint32_t> address_distribution;
    std::uniform_int_distribution<std::uint32_t> value_distribution;
    for (std::size_t iteration = 0; iteration < 5000; ++iteration) {
        const auto address = address_distribution(random);
        const auto value = value_distribution(random);
        switch (iteration % 3) {
        case 0:
            static_cast<void>(bus.read8(address));
            static_cast<void>(bus.write8(address, static_cast<std::uint8_t>(value)));
            break;
        case 1:
            static_cast<void>(bus.read16(address));
            static_cast<void>(bus.write16(address, static_cast<std::uint16_t>(value)));
            break;
        default:
            static_cast<void>(bus.read32(address));
            static_cast<void>(bus.write32(address, value));
            break;
        }
    }
}

TEST_CASE("scheduler ordena eventos por ciclo e mantém FIFO em empates") {
    ccl::gba::Scheduler scheduler;
    const auto late = scheduler.schedule(20, {ccl::gba::SchedulerEventType::timer, 20});
    const auto first = scheduler.schedule(10, {ccl::gba::SchedulerEventType::scanline, 1});
    const auto second = scheduler.schedule(10, {ccl::gba::SchedulerEventType::dma, 2});
    CHECK(late != first);
    CHECK(first != second);
    REQUIRE(scheduler.next_deadline().has_value());
    CHECK(*scheduler.next_deadline() == 10);
    CHECK(scheduler.pop_due(9).empty());

    const auto due = scheduler.pop_due(10);
    REQUIRE(due.size() == 2);
    CHECK(due[0].handle == first);
    CHECK(due[0].event.payload == 1);
    CHECK(due[1].handle == second);
    CHECK(due[1].event.payload == 2);
    CHECK(scheduler.current_time() == 10);
    CHECK(scheduler.next_deadline() == 20);
}

TEST_CASE("scheduler cancela handles e next_deadline vazio retorna nullopt") {
    ccl::gba::Scheduler scheduler;
    CHECK_FALSE(scheduler.next_deadline().has_value());
    const auto cancelled = scheduler.schedule(5, {ccl::gba::SchedulerEventType::timer, 7});
    CHECK(scheduler.cancel(cancelled));
    CHECK_FALSE(scheduler.cancel(cancelled));
    CHECK_FALSE(scheduler.next_deadline().has_value());
}

TEST_CASE("scheduler permite reagendar enquanto o chamador consome pop_due") {
    ccl::gba::Scheduler scheduler;
    static_cast<void>(scheduler.schedule(0, {ccl::gba::SchedulerEventType::scanline, 0}));
    std::size_t event_count = 0;
    for (std::uint64_t now = 0; now < ccl::gba::cycles_per_frame; now += ccl::gba::cycles_per_scanline) {
        const auto due = scheduler.pop_due(now);
        for (const auto& event : due) {
            ++event_count;
            static_cast<void>(scheduler.schedule(now + ccl::gba::cycles_per_scanline,
                {ccl::gba::SchedulerEventType::scanline, event.event.payload + 1}));
        }
    }
    CHECK(event_count == 228);
    CHECK(scheduler.next_deadline() == ccl::gba::cycles_per_frame);
}

TEST_CASE("agendar no passado aborta explicitamente") {
    const auto child = fork();
    REQUIRE(child >= 0);
    if (child == 0) {
        const rlimit no_core{0, 0};
        static_cast<void>(setrlimit(RLIMIT_CORE, &no_core));
        static_cast<void>(std::freopen("/dev/null", "w", stderr));
        ccl::gba::Scheduler scheduler;
        static_cast<void>(scheduler.pop_due(10));
        static_cast<void>(scheduler.schedule(9, {ccl::gba::SchedulerEventType::timer, 0}));
        _exit(0);
    }
    int status = 0;
    REQUIRE(waitpid(child, &status, 0) == child);
    CHECK(WIFSIGNALED(status));
    CHECK(WTERMSIG(status) == SIGABRT);
}
