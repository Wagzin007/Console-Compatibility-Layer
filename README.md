# CCL — Console Compatibility Layer

CCL é um projeto experimental de uma plataforma modular de emulação para Linux x86-64. O primeiro core planejado é o do Game Boy Advance.

**Estado atual:** fundação M0 em andamento. Ainda não existe emulador: o programa atual apenas informa a versão e mostra a ajuda. Nenhum jogo ou ROM é carregado.

## Build e testes

Dependências: CMake 3.20 ou superior, compilador C++20 e o pacote `doctest` fornecido pelo sistema (no Arch Linux: `pacman -S doctest`).

```sh
cmake --preset debug-asan
cmake --build --preset debug-asan
ctest --preset debug-asan --output-on-failure
./build/debug-asan/ccl-run --version

cmake --preset release
cmake --build --preset release
```

O preset `debug-asan` habilita AddressSanitizer e UndefinedBehaviorSanitizer. Nesse preset, a opção `detect_leaks=0` desativa apenas LeakSanitizer, que encerra com erro neste ambiente quando executado sob `ptrace`; a detecção de leaks não é validada aqui. O preset `release` configura uma build otimizada.

## Uso

```sh
ccl-run --version
ccl-run --help
```

As opções acima são as únicas implementadas nesta etapa. O programa não carrega ROMs.

## Aviso legal

Este projeto não distribui ROMs, BIOS, ISOs ou outros assets proprietários. Use somente conteúdo que você tenha direito de usar. A licença do projeto é GNU GPL versão 3; consulte [LICENSE](LICENSE).

## Marcos

| Marco | Escopo planejado | Estado |
|---|---|---|
| M0 | Fundação, build e CLI mínimo | Em andamento |
| M1 | Estrutura inicial do core GBA e leitura de imagem | Não iniciado |
| M2 | CPU ARM7TDMI | Não iniciado |
| M3 | Bus e mapa de memória GBA | Não iniciado |
| M4 | Temporização e scheduler | Não iniciado |
| M5 | PPU e framebuffer | Não iniciado |
| M6 | Entrada e áudio | Não iniciado |
| M7 | Saves e estado de execução | Não iniciado |
| M8 | Interface host e integração de frontend | Não iniciado |
| M9 | Testes de compatibilidade e desempenho | Não iniciado |
| M10 | Preparação de versão | Não iniciado |

## Decisões e perguntas abertas

As decisões arquiteturais aceitas ou propostas estão em [`docs/adr`](docs/adr/). Permanecem em aberto: SDL2 ou SDL3 para um frontend futuro; meta de desempenho na máquina de teste; e quais suítes de terceiros poderão ser usadas após checagem de licença. Veja [`docs/open-questions.md`](docs/open-questions.md).
