#include "common/version.hpp"

#include <iostream>
#include <string_view>

namespace {

void print_help() {
    std::cout << "Uso: ccl-run [--version|--help]\n"
              << "Mostra a versão ou esta ajuda. Nenhuma ROM é carregada.\n";
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc == 1) {
        print_help();
        return 0;
    }

    const std::string_view argument{argv[1]};
    if (argc == 2 && argument == "--version") {
        std::cout << ccl::version() << '\n';
        return 0;
    }
    if (argc == 2 && argument == "--help") {
        print_help();
        return 0;
    }

    std::cerr << "ccl-run: argumento inválido: " << argument
              << "\nUse --help para ver as opções disponíveis.\n";
    return 2;
}
