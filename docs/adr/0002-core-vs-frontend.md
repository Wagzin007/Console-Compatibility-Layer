# ADR 0002: Core e frontend

- **Contexto:** O core deve ser utilizável sem interface gráfica e sem impor dependências de UI.
- **Opções:** Embutir a interface no core; carregar cores como plugins; manter um executável host separado.
- **Decisão:** O core será uma biblioteca estática headless, sem SDL e sem Qt. Um futuro Launcher em Qt Widgets será um processo separado que inicia `ccl-run`. O registro dos cores será feito em tempo de compilação, sem plugins `dlopen`.
- **Consequências:** O core permanece sem dependência de interface gráfica; a comunicação com um launcher futuro ocorrerá por processo.
- **Status:** Aceita.
