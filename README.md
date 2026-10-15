# CCL — Console Compatibility Layer

CCL é um projeto experimental de uma plataforma modular de emulação para Linux x86-64; o primeiro módulo de console do CCL-C é o GBA.

### Glossário

- **CCL-C:** Core do projeto, que executa a lógica principal e a CPU do console-alvo e coordena os subsistemas.
- **CCL-G (Gummy):** subsistema gráfico integrado ao CCL-C, separado em adaptadores por console, infraestrutura gráfica compartilhada e backends.
- **Gummy OpenGL:** primeiro backend gráfico do CCL-G.
- **Gummy Vulkan:** backend gráfico futuro do CCL-G.

**Estado atual:** M1 concluído. `ccl-run info` lê uma ROM GBA local somente para exibir os metadados do cabeçalho. Ainda não existe emulador e nenhum jogo é executado.

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
ccl-run info caminho/para/arquivo.gba
```

`info` não modifica o arquivo. Cabeçalhos com anomalias conhecidas são exibidos com avisos; arquivos vazios, truncados ou acima de 32 MiB são rejeitados. O programa ainda não executa ROMs.

## Aviso legal

Este projeto não distribui ROMs, BIOS, ISOs ou outros assets proprietários. Use somente conteúdo que você tenha direito de usar. A licença do projeto é GNU GPL versão 3; consulte [LICENSE](LICENSE).

## Marcos

| Marco | Escopo planejado | Estado |
|---|---|---|
| M0 | Fundação | Concluído |
| M1 | Loader/header e `ccl-run` headless | Concluído |
| M2 | Bus e scheduler | Em andamento |
| M3 | Fatia vertical (ARM mínimo, modo 3 e hash de framebuffer) | Não iniciado |
| M4 | CPU completa no CCL-C | Não iniciado |
| M5 | Interrupções, timers, DMA, BIOS-HLE e PPU | Não iniciado |
| M6 | SDL2 e áudio | Não iniciado |
| M7 | Saves e RTC/GPIO | Não iniciado |
| M8 | Registry/Detector | Não iniciado |
| M9 | Launcher Qt | Não iniciado |
| M10 | Benchmarks e decisão sobre JIT | Não iniciado |

## Decisões e perguntas abertas

As decisões arquiteturais aceitas ou propostas estão em [`docs/adr`](docs/adr/). Permanecem em aberto: SDL2 ou SDL3 para um frontend futuro; meta de desempenho na máquina de teste; e quais suítes de terceiros poderão ser usadas após checagem de licença. Veja [`docs/open-questions.md`](docs/open-questions.md).
