#include "cli/cli.hpp"

#include "common/file.hpp"
#include "common/version.hpp"
#include "gba/rom_header.hpp"

#include <filesystem>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <utility>

namespace ccl::cli {
namespace {

void print_help(std::ostream& out) {
    out << "Uso: ccl-run [--version|--help]\n"
        << "     ccl-run info <arquivo>\n"
        << "Mostra a versão, esta ajuda ou os metadados do cabeçalho de uma ROM GBA.\n";
}

std::string hex_byte(std::uint8_t value) {
    std::ostringstream formatted;
    formatted << "0x" << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
              << static_cast<unsigned int>(value);
    return formatted.str();
}

void print_info(
    const std::filesystem::path& path,
    std::size_t size,
    const gba::RomHeader& header,
    std::ostream& out) {
    out << "Arquivo: " << path.filename().string() << '\n'
        << "Tamanho: " << size << " bytes\n"
        << "Título: " << header.title << '\n'
        << "Código do jogo: " << header.game_code << '\n'
        << "Fabricante: " << header.maker_code << '\n'
        << "Versão do software: " << static_cast<unsigned int>(header.software_version) << '\n'
        << "Byte fixo: " << hex_byte(header.fixed_value)
        << (header.fixed_value == 0x96 ? " (ok)\n" : " (divergente)\n")
        << "Checksum: armazenado " << hex_byte(header.checksum_stored)
        << ", calculado " << hex_byte(header.checksum_calculated)
        << (header.checksum_stored == header.checksum_calculated ? " (ok)\n" : " (divergente)\n");

    if (header.warnings.empty()) {
        out << "Avisos: nenhum\n";
        return;
    }
    out << "Avisos:\n";
    for (const auto& warning : header.warnings) {
        out << "- " << warning << '\n';
    }
}

int print_usage_error(std::ostream& err, std::string_view detail) {
    if (!detail.empty()) {
        err << "ccl-run: " << detail << '\n';
    }
    err << "Uso: ccl-run [--version|--help] ou ccl-run info <arquivo>\n";
    return 2;
}

} // namespace

int run(const std::vector<std::string_view>& args, std::ostream& out, std::ostream& err) {
    if (args.empty() || (args.size() == 1 && args[0] == "--help")) {
        print_help(out);
        return 0;
    }
    if (args.size() == 1 && args[0] == "--version") {
        out << version() << '\n';
        return 0;
    }
    if (!args.empty() && args[0] == "info") {
        if (args.size() != 2 || args[1].empty()) {
            return print_usage_error(err, "info requer exatamente um caminho de arquivo");
        }

        const std::filesystem::path path{args[1]};
        auto file_result = read_file(path, gba::max_rom_size);
        if (!file_result.has_value()) {
            err << "ccl-run: erro de leitura: " << file_result.error().message << '\n';
            return 1;
        }

        auto contents = std::move(file_result).value();
        auto header_result = gba::parse_header(contents);
        if (!header_result.has_value()) {
            err << "ccl-run: ROM inválida: " << header_result.error().message << '\n';
            return 1;
        }

        print_info(path, contents.size(), header_result.value(), out);
        return 0;
    }

    if (args[0] == "--help" || args[0] == "--version") {
        return print_usage_error(err, "opção não aceita argumentos adicionais");
    }
    return print_usage_error(err, "comando ou opção inválida: " + std::string(args[0]));
}

} // namespace ccl::cli
