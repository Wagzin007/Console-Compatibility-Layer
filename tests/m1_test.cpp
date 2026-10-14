#include <doctest/doctest.h>

#include "cli/cli.hpp"
#include "common/file.hpp"
#include "gba/rom_header.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <random>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr std::size_t title_offset = 0xA0;
constexpr std::size_t fixed_value_offset = 0xB2;
constexpr std::size_t unit_code_offset = 0xB3;
constexpr std::size_t software_version_offset = 0xBC;
constexpr std::size_t checksum_offset = 0xBD;

class TemporaryDirectory {
public:
    TemporaryDirectory() {
        const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
        path_ = std::filesystem::temp_directory_path() / ("ccl-m1-" + std::to_string(timestamp));
        std::filesystem::create_directory(path_);
    }

    ~TemporaryDirectory() {
        std::error_code ignored;
        std::filesystem::remove_all(path_, ignored);
    }

    [[nodiscard]] const std::filesystem::path& path() const noexcept { return path_; }

private:
    std::filesystem::path path_;
};

std::uint8_t independent_checksum(const std::vector<std::uint8_t>& bytes) {
    unsigned int sum = 0x19U;
    for (std::size_t offset = title_offset; offset <= software_version_offset; ++offset) {
        sum += bytes[offset];
    }
    return static_cast<std::uint8_t>(0U - sum);
}

void refresh_checksum(std::vector<std::uint8_t>& bytes) {
    bytes[checksum_offset] = independent_checksum(bytes);
}

std::vector<std::uint8_t> make_valid_rom() {
    std::vector<std::uint8_t> bytes(ccl::gba::rom_header_size, 0);
    bytes[0x00] = 0xE3;
    bytes[0x01] = 0xA0;
    bytes[0x02] = 0x20;
    bytes[0x03] = 0x01;
    constexpr std::string_view title = "ABCDEFGHIJKL";
    for (std::size_t index = 0; index < title.size(); ++index) {
        bytes[title_offset + index] = static_cast<std::uint8_t>(title[index]);
    }
    bytes[0xAC] = 'A';
    bytes[0xAD] = 'B';
    bytes[0xAE] = '1';
    bytes[0xAF] = '2';
    bytes[0xB0] = 'X';
    bytes[0xB1] = 'Y';
    bytes[fixed_value_offset] = 0x96;
    bytes[unit_code_offset] = 0x00;
    for (std::size_t offset = 0xB4; offset <= 0xBB; ++offset) {
        bytes[offset] = static_cast<std::uint8_t>(0x31 + offset - 0xB4);
    }
    bytes[software_version_offset] = 0x42;
    // Calculado independentemente: Python: (-(0x19 + sum(bytes[0xA0:0xBD]))) & 0xFF == 0x86.
    bytes[checksum_offset] = 0x86;
    return bytes;
}

bool has_warning(const ccl::gba::RomHeader& header, std::string_view text) {
    for (const auto& warning : header.warnings) {
        if (warning.find(text) != std::string::npos) {
            return true;
        }
    }
    return false;
}

void write_bytes(const std::filesystem::path& path, const std::vector<std::uint8_t>& bytes) {
    std::ofstream output(path, std::ios::binary);
    output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}

} // namespace

TEST_CASE("o cabeçalho GBA lê cada campo pelo offset definido") {
    const auto bytes = make_valid_rom();
    const auto parsed = ccl::gba::parse_header(bytes);

    REQUIRE(parsed.has_value());
    CHECK(parsed.value().entry_instruction == 0x0120A0E3U);
    CHECK(parsed.value().title == "ABCDEFGHIJKL");
    CHECK(parsed.value().game_code == "AB12");
    CHECK(parsed.value().maker_code == "XY");
    CHECK(parsed.value().fixed_value == 0x96);
    CHECK(parsed.value().unit_code == 0x00);
    CHECK(parsed.value().software_version == 0x42);
    CHECK(parsed.value().checksum_stored == 0x86);
    CHECK(parsed.value().checksum_calculated == 0x86);
    CHECK(parsed.value().warnings.empty());
}

TEST_CASE("tamanho da ROM valida erros, mínimo e limite sem alocar ROM grande") {
    CHECK(ccl::gba::validate_rom_size(0).has_value());
    CHECK(ccl::gba::validate_rom_size(191).has_value());
    CHECK_FALSE(ccl::gba::validate_rom_size(192).has_value());
    CHECK_FALSE(ccl::gba::validate_rom_size(ccl::gba::max_rom_size).has_value());
    CHECK(ccl::gba::validate_rom_size(ccl::gba::max_rom_size + 1).has_value());

    const std::vector<std::uint8_t> empty;
    const std::vector<std::uint8_t> short_header(191);
    const auto empty_result = ccl::gba::parse_header(empty);
    const auto short_result = ccl::gba::parse_header(short_header);
    REQUIRE_FALSE(empty_result.has_value());
    REQUIRE_FALSE(short_result.has_value());
    CHECK(empty_result.error().message.find("vazio") != std::string::npos);
    CHECK(short_result.error().message.find("incompleto") != std::string::npos);
    CHECK(ccl::gba::parse_header(make_valid_rom()).has_value());
}

TEST_CASE("cada anomalia do cabeçalho gera apenas seu aviso") {
    SUBCASE("byte fixo divergente") {
        auto bytes = make_valid_rom();
        bytes[fixed_value_offset] = 0x95;
        refresh_checksum(bytes);
        const auto parsed = ccl::gba::parse_header(bytes);
        REQUIRE(parsed.has_value());
        REQUIRE(parsed.value().warnings.size() == 1);
        CHECK(has_warning(parsed.value(), "byte fixo"));
    }

    SUBCASE("checksum divergente") {
        auto bytes = make_valid_rom();
        bytes[checksum_offset] ^= 0x01;
        const auto parsed = ccl::gba::parse_header(bytes);
        REQUIRE(parsed.has_value());
        REQUIRE(parsed.value().warnings.size() == 1);
        CHECK(has_warning(parsed.value(), "checksum"));
    }

    SUBCASE("código da unidade divergente") {
        auto bytes = make_valid_rom();
        bytes[unit_code_offset] = 0x01;
        refresh_checksum(bytes);
        const auto parsed = ccl::gba::parse_header(bytes);
        REQUIRE(parsed.has_value());
        REQUIRE(parsed.value().warnings.size() == 1);
        CHECK(has_warning(parsed.value(), "unidade"));
    }

    SUBCASE("título contém byte não imprimível") {
        auto bytes = make_valid_rom();
        bytes[title_offset + 2] = 0x80;
        refresh_checksum(bytes);
        const auto parsed = ccl::gba::parse_header(bytes);
        REQUIRE(parsed.has_value());
        REQUIRE(parsed.value().warnings.size() == 1);
        CHECK(has_warning(parsed.value(), "não ASCII"));
        CHECK(parsed.value().title == "AB?DEFGHIJKL");
    }

    SUBCASE("código do jogo contém ESC") {
        auto bytes = make_valid_rom();
        bytes[0xAC] = 0x1B;
        refresh_checksum(bytes);
        const auto parsed = ccl::gba::parse_header(bytes);
        REQUIRE(parsed.has_value());
        REQUIRE(parsed.value().warnings.size() == 1);
        CHECK(parsed.value().game_code == "?B12");
        CHECK(has_warning(parsed.value(), "código do jogo contém bytes não imprimíveis"));
    }

    SUBCASE("código do fabricante contém BEL") {
        auto bytes = make_valid_rom();
        bytes[0xB0] = 0x07;
        refresh_checksum(bytes);
        const auto parsed = ccl::gba::parse_header(bytes);
        REQUIRE(parsed.has_value());
        REQUIRE(parsed.value().warnings.size() == 1);
        CHECK(parsed.value().maker_code == "?Y");
        CHECK(has_warning(parsed.value(), "código do fabricante contém bytes não imprimíveis"));
    }

    SUBCASE("NUL nos dois códigos vira ponto de interrogação") {
        auto bytes = make_valid_rom();
        bytes[0xAE] = 0x00;
        bytes[0xB1] = 0x00;
        refresh_checksum(bytes);
        const auto parsed = ccl::gba::parse_header(bytes);
        REQUIRE(parsed.has_value());
        REQUIRE(parsed.value().warnings.size() == 2);
        CHECK(parsed.value().game_code == "AB?2");
        CHECK(parsed.value().maker_code == "X?");
        CHECK(has_warning(parsed.value(), "código do jogo contém bytes não imprimíveis"));
        CHECK(has_warning(parsed.value(), "código do fabricante contém bytes não imprimíveis"));
    }

    SUBCASE("tamanho não é múltiplo de quatro") {
        auto bytes = make_valid_rom();
        bytes.push_back(0x00);
        const auto parsed = ccl::gba::parse_header(bytes);
        REQUIRE(parsed.has_value());
        REQUIRE(parsed.value().warnings.size() == 1);
        CHECK(has_warning(parsed.value(), "múltiplo de 4"));
    }
}

TEST_CASE("título com padding NUL termina no primeiro byte zero") {
    auto bytes = make_valid_rom();
    bytes[title_offset + 3] = 0;
    refresh_checksum(bytes);
    const auto parsed = ccl::gba::parse_header(bytes);

    REQUIRE(parsed.has_value());
    CHECK(parsed.value().title == "ABC");
    CHECK(parsed.value().warnings.empty());
}

TEST_CASE("parser tolera truncamentos e buffers aleatórios sem acessar fora dos limites") {
    const auto valid = make_valid_rom();
    for (std::size_t size = 0; size <= 200; ++size) {
        const std::span<const std::uint8_t> truncated{valid.data(), size};
        const auto parsed = ccl::gba::parse_header(truncated);
        CHECK(parsed.has_value() == (size >= ccl::gba::rom_header_size));
    }

    std::mt19937 random{0xC0FFEEU};
    std::uniform_int_distribution<std::size_t> length_distribution(0, 256);
    std::uniform_int_distribution<unsigned int> byte_distribution(0, 255);
    for (std::size_t iteration = 0; iteration < 1000; ++iteration) {
        std::vector<std::uint8_t> random_bytes(length_distribution(random));
        for (auto& byte : random_bytes) {
            byte = static_cast<std::uint8_t>(byte_distribution(random));
        }
        static_cast<void>(ccl::gba::parse_header(random_bytes));
    }
}

TEST_CASE("read_file rejeita caminho inexistente, diretório e tamanho acima do limite") {
    TemporaryDirectory temporary;
    const auto missing = ccl::read_file(temporary.path() / "missing.gba", ccl::gba::max_rom_size);
    const auto directory = ccl::read_file(temporary.path(), ccl::gba::max_rom_size);

    REQUIRE_FALSE(missing.has_value());
    CHECK(missing.error().message.find("não encontrado") != std::string::npos);
    REQUIRE_FALSE(directory.has_value());
    CHECK(directory.error().message.find("diretório") != std::string::npos);

    const auto rom_path = temporary.path() / "small.gba";
    write_bytes(rom_path, make_valid_rom());
    const auto too_small_limit = ccl::read_file(rom_path, 191);
    REQUIRE_FALSE(too_small_limit.has_value());
    CHECK(too_small_limit.error().message.find("excede o limite") != std::string::npos);
}

TEST_CASE("ccl-run info mostra metadados sem alterar o arquivo") {
    TemporaryDirectory temporary;
    const auto path = temporary.path() / "synthetic.gba";
    const auto bytes = make_valid_rom();
    write_bytes(path, bytes);
    REQUIRE(std::filesystem::exists(path));
    const auto modified_before = std::filesystem::last_write_time(path);

    std::ostringstream out;
    std::ostringstream err;
    const int exit_code = ccl::cli::run({"info", path.string()}, out, err);

    CHECK(exit_code == 0);
    CHECK(err.str().empty());
    CHECK(out.str().find("Arquivo: synthetic.gba") != std::string::npos);
    CHECK(out.str().find("Tamanho: 192 bytes") != std::string::npos);
    CHECK(out.str().find("Título: ABCDEFGHIJKL") != std::string::npos);
    CHECK(out.str().find("Código do jogo: AB12") != std::string::npos);
    CHECK(out.str().find("Fabricante: XY") != std::string::npos);
    CHECK(out.str().find("Versão do software: 66") != std::string::npos);
    CHECK(out.str().find("Byte fixo: 0x96 (ok)") != std::string::npos);
    CHECK(out.str().find("Checksum: armazenado 0x86, calculado 0x86 (ok)") != std::string::npos);
    CHECK(out.str().find("Avisos: nenhum") != std::string::npos);

    const auto modified_after = std::filesystem::last_write_time(path);
    const auto contents_after = ccl::read_file(path, ccl::gba::max_rom_size);
    CHECK(modified_after == modified_before);
    REQUIRE(contents_after.has_value());
    CHECK(contents_after.value() == bytes);
}

TEST_CASE("ccl-run info retorna sucesso para ROM com avisos") {
    TemporaryDirectory temporary;
    const auto path = temporary.path() / "warning.gba";
    auto bytes = make_valid_rom();
    bytes[fixed_value_offset] = 0x95;
    refresh_checksum(bytes);
    write_bytes(path, bytes);

    std::ostringstream out;
    std::ostringstream err;
    CHECK(ccl::cli::run({"info", path.string()}, out, err) == 0);
    CHECK(out.str().find("Byte fixo: 0x95 (divergente)") != std::string::npos);
    CHECK(out.str().find("- byte fixo diferente do esperado") != std::string::npos);
    CHECK(err.str().empty());
}

TEST_CASE("CLI preserva códigos de saída e documenta info na ajuda") {
    std::ostringstream out;
    std::ostringstream err;
    CHECK(ccl::cli::run({"--help"}, out, err) == 0);
    CHECK(out.str().find("ccl-run info <arquivo>") != std::string::npos);
    CHECK(err.str().empty());

    out.str({});
    CHECK(ccl::cli::run({"--version"}, out, err) == 0);
    CHECK(out.str() == std::string(CCL_VERSION) + "\n");

    out.str({});
    err.str({});
    CHECK(ccl::cli::run({"info"}, out, err) == 2);
    CHECK(err.str().find("requer exatamente") != std::string::npos);

    out.str({});
    err.str({});
    CHECK(ccl::cli::run({"info", "arquivo.gba", "extra"}, out, err) == 2);

    out.str({});
    err.str({});
    CHECK(ccl::cli::run({"comando-inexistente"}, out, err) == 2);
}

TEST_CASE("CLI retorna erro de leitura e ROM inválida") {
    TemporaryDirectory temporary;
    std::ostringstream out;
    std::ostringstream err;

    const auto missing_path = temporary.path() / "missing.gba";
    CHECK(ccl::cli::run({"info", missing_path.string()}, out, err) == 1);
    CHECK(err.str().find("erro de leitura") != std::string::npos);

    const auto short_path = temporary.path() / "truncated.gba";
    auto truncated = make_valid_rom();
    truncated.resize(191);
    write_bytes(short_path, truncated);
    out.str({});
    err.str({});
    CHECK(ccl::cli::run({"info", short_path.string()}, out, err) == 1);
    CHECK(err.str().find("ROM inválida") != std::string::npos);
}
