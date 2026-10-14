#include "gba/rom_header.hpp"

namespace ccl::gba {
namespace {

constexpr std::size_t entry_offset = 0x00;
constexpr std::size_t title_offset = 0xA0;
constexpr std::size_t title_size = 12;
constexpr std::size_t game_code_offset = 0xAC;
constexpr std::size_t maker_code_offset = 0xB0;
constexpr std::size_t fixed_value_offset = 0xB2;
constexpr std::size_t unit_code_offset = 0xB3;
constexpr std::size_t software_version_offset = 0xBC;
constexpr std::size_t checksum_offset = 0xBD;
constexpr std::uint8_t expected_fixed_value = 0x96;
constexpr std::uint8_t expected_unit_code = 0x00;

std::uint32_t read_u32_le(std::span<const std::uint8_t> bytes, std::size_t offset) {
    // parse_header validates the complete 192-byte header before any field is read.
    return static_cast<std::uint32_t>(bytes[offset])
        | (static_cast<std::uint32_t>(bytes[offset + 1]) << 8U)
        | (static_cast<std::uint32_t>(bytes[offset + 2]) << 16U)
        | (static_cast<std::uint32_t>(bytes[offset + 3]) << 24U);
}

std::string read_sanitized_code(
    std::span<const std::uint8_t> bytes,
    std::size_t offset,
    std::size_t length,
    bool& has_non_printable) {
    std::string result;
    result.reserve(length);
    for (std::size_t index = 0; index < length; ++index) {
        const auto byte = bytes[offset + index];
        if (byte < 0x20 || byte > 0x7E) {
            has_non_printable = true;
            result.push_back('?');
        } else {
            result.push_back(static_cast<char>(byte));
        }
    }
    return result;
}

std::uint8_t calculate_checksum(std::span<const std::uint8_t> bytes) {
    unsigned int sum = 0x19U;
    for (std::size_t offset = title_offset; offset <= software_version_offset; ++offset) {
        sum += bytes[offset];
    }
    return static_cast<std::uint8_t>(0U - sum);
}

} // namespace

std::optional<Error> validate_rom_size(std::size_t size) {
    if (size == 0) {
        return Error{"arquivo ROM vazio"};
    }
    if (size < rom_header_size) {
        return Error{"cabeçalho GBA incompleto: esperado no mínimo 192 bytes"};
    }
    if (size > max_rom_size) {
        return Error{"ROM excede o limite de 32 MiB"};
    }
    return std::nullopt;
}

Result<RomHeader, Error> parse_header(std::span<const std::uint8_t> bytes) {
    if (const auto size_error = validate_rom_size(bytes.size())) {
        return Result<RomHeader, Error>::failure(*size_error);
    }

    RomHeader header;
    header.entry_instruction = read_u32_le(bytes, entry_offset);
    bool game_code_has_non_printable = false;
    bool maker_code_has_non_printable = false;
    header.game_code = read_sanitized_code(bytes, game_code_offset, 4, game_code_has_non_printable);
    header.maker_code = read_sanitized_code(bytes, maker_code_offset, 2, maker_code_has_non_printable);
    header.fixed_value = bytes[fixed_value_offset];
    header.unit_code = bytes[unit_code_offset];
    header.software_version = bytes[software_version_offset];
    header.checksum_stored = bytes[checksum_offset];
    header.checksum_calculated = calculate_checksum(bytes);

    bool title_ended = false;
    bool title_has_non_printable = false;
    for (std::size_t index = 0; index < title_size; ++index) {
        const auto byte = bytes[title_offset + index];
        if (byte == 0) {
            title_ended = true;
            continue;
        }
        if (byte < 0x20 || byte > 0x7E) {
            title_has_non_printable = true;
            if (!title_ended) {
                header.title.push_back('?');
            }
            continue;
        }
        if (!title_ended) {
            header.title.push_back(static_cast<char>(byte));
        }
    }

    if (header.fixed_value != expected_fixed_value) {
        header.warnings.emplace_back("byte fixo diferente do esperado (0x96)");
    }
    if (header.checksum_stored != header.checksum_calculated) {
        header.warnings.emplace_back("checksum divergente");
    }
    if (header.unit_code != expected_unit_code) {
        header.warnings.emplace_back("código da unidade diferente do esperado (0x00)");
    }
    if (title_has_non_printable) {
        header.warnings.emplace_back("título contém bytes não ASCII imprimíveis");
    }
    if (game_code_has_non_printable) {
        header.warnings.emplace_back("código do jogo contém bytes não imprimíveis");
    }
    if (maker_code_has_non_printable) {
        header.warnings.emplace_back("código do fabricante contém bytes não imprimíveis");
    }
    if (bytes.size() % 4 != 0) {
        header.warnings.emplace_back("tamanho da ROM não é múltiplo de 4 bytes");
    }

    return Result<RomHeader, Error>::success(std::move(header));
}

} // namespace ccl::gba
