#include "common/file.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <limits>

namespace ccl {
namespace {

Error file_error(const std::filesystem::path& path, const std::string& message) {
    return Error{"'" + path.string() + "': " + message};
}

} // namespace

Result<std::vector<std::uint8_t>, Error> read_file(
    const std::filesystem::path& path,
    std::size_t max_bytes) {
    std::error_code status_error;
    const auto status = std::filesystem::status(path, status_error);
    if (status_error || !std::filesystem::exists(status)) {
        return Result<std::vector<std::uint8_t>, Error>::failure(
            file_error(path, "arquivo não encontrado ou inacessível"));
    }
    if (std::filesystem::is_directory(status)) {
        return Result<std::vector<std::uint8_t>, Error>::failure(
            file_error(path, "o caminho é um diretório"));
    }
    if (!std::filesystem::is_regular_file(status)) {
        return Result<std::vector<std::uint8_t>, Error>::failure(
            file_error(path, "o caminho não é um arquivo regular"));
    }

    std::error_code size_error;
    const auto size = std::filesystem::file_size(path, size_error);
    if (size_error) {
        return Result<std::vector<std::uint8_t>, Error>::failure(
            file_error(path, "não foi possível consultar o tamanho"));
    }
    if (size > max_bytes) {
        return Result<std::vector<std::uint8_t>, Error>::failure(
            file_error(path, "o arquivo excede o limite de " + std::to_string(max_bytes) + " bytes"));
    }

    if (max_bytes == std::numeric_limits<std::size_t>::max()) {
        return Result<std::vector<std::uint8_t>, Error>::failure(
            file_error(path, "o limite de leitura não pode ser representado"));
    }

    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return Result<std::vector<std::uint8_t>, Error>::failure(
            file_error(path, "não foi possível abrir para leitura"));
    }

    const std::size_t read_limit = max_bytes + 1;
    std::vector<std::uint8_t> contents;
    const auto reserve_size = std::min<std::size_t>(static_cast<std::size_t>(size), read_limit);
    if (reserve_size > contents.max_size()) {
        return Result<std::vector<std::uint8_t>, Error>::failure(
            file_error(path, "o arquivo excede a capacidade de leitura"));
    }
    contents.reserve(reserve_size);

    std::array<char, 8192> buffer{};
    while (contents.size() < read_limit) {
        const auto request_size = std::min(buffer.size(), read_limit - contents.size());
        input.read(buffer.data(), static_cast<std::streamsize>(request_size));
        const auto bytes_read = input.gcount();
        for (std::streamsize i = 0; i < bytes_read; ++i) {
            contents.push_back(static_cast<std::uint8_t>(static_cast<unsigned char>(buffer[static_cast<std::size_t>(i)])));
        }

        if (input.bad()) {
            return Result<std::vector<std::uint8_t>, Error>::failure(
                file_error(path, "erro durante a leitura"));
        }
        if (bytes_read < static_cast<std::streamsize>(request_size)) {
            if (!input.eof()) {
                return Result<std::vector<std::uint8_t>, Error>::failure(
                    file_error(path, "erro durante a leitura"));
            }
            break;
        }
    }

    if (contents.size() > max_bytes) {
        return Result<std::vector<std::uint8_t>, Error>::failure(
            file_error(path, "o arquivo cresceu além do limite de " + std::to_string(max_bytes) + " bytes"));
    }
    return Result<std::vector<std::uint8_t>, Error>::success(std::move(contents));
}

} // namespace ccl
