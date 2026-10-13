# ADR 0007: Linguagem e build

- **Contexto:** A fundação precisa de uma linguagem, build reproduzível e testes automatizados disponíveis no ambiente de desenvolvimento.
- **Opções:** C++ sem padrão fixo; adotar C++20 com CMake e CTest.
- **Decisão:** Usar C++20, CMake 3.20 ou superior com presets, CTest, doctest e ASan/UBSan. `Result<T, E>` será fino e poderá ser trocado por `std::expected` no futuro.
- **Consequências:** Builds de debug instrumentadas e release ficam padronizadas; doctest é dependência fornecida pelo sistema, sem cópia vendorizada.
- **Status:** Aceita.
