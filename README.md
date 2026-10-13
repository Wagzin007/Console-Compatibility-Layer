# CCL — Console Compatibility Layer

CCL é um projeto experimental de uma plataforma modular de emulação para Linux x86-64. O primeiro core planejado é o do Game Boy Advance.

**Estado atual:** fundação M0 em andamento. Ainda não existe emulador: o programa atual apenas informa a versão e mostra a ajuda. Nenhum jogo ou ROM é carregado.

## Build e testes

Dependências: CMake 3.21 ou superior, compilador C++20 e o pacote `doctest` fornecido pelo sistema (no Arch Linux: `pacman -S doctest`).

```sh
cmake --preset debug-asan
cmake --build --preset debug-asan
ctest --preset debug-asan --output-on-failure
./build/debug-asan/ccl-run --version

cmake --preset release
cmake --build --preset release
```

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
| M0 | Fundação | Em andamento |
| M1 | Loader/header e `ccl-run` headless | Não iniciado |
| M2 | Bus e scheduler | Não iniciado |
| M3 | Fatia vertical (ARM mínimo, modo 3 e hash de framebuffer) | Não iniciado |
| M4 | CPU completa | Não iniciado |
| M5 | Interrupções, timers, DMA, BIOS-HLE e PPU | Não iniciado |
| M6 | SDL2 e áudio | Não iniciado |
| M7 | Saves e RTC/GPIO | Não iniciado |
| M8 | Registry/Detector | Não iniciado |
| M9 | Launcher Qt | Não iniciado |
| M10 | Benchmarks e decisão sobre JIT | Não iniciado |

## Decisões e perguntas abertas

As decisões arquiteturais aceitas ou propostas estão em [`docs/adr`](docs/adr/). Permanecem em aberto: SDL2 ou SDL3 para um frontend futuro; meta de desempenho na máquina de teste; e quais suítes de terceiros poderão ser usadas após checagem de licença. Veja [`docs/open-questions.md`](docs/open-questions.md).
