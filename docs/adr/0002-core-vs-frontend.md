# ADR 0002: CCL-C, CCL-G e frontend

- **Contexto:** O CCL-C é o próprio Core do projeto: executa a lógica principal e a CPU do console-alvo e coordena os subsistemas. Não existe um componente separado chamado CCL Core acima dele. O CCL-C deve ser utilizável sem interface gráfica e sem impor dependências de UI.
- **Opções:** Embutir a interface no CCL-C; carregar implementações de console como plugins; manter um executável host separado.
- **Decisão:** O CCL-C será uma biblioteca estática headless, sem SDL e sem Qt. Um futuro Launcher em Qt Widgets será um processo separado que inicia `ccl-run`. O registro dos consoles será feito em tempo de compilação, sem plugins `dlopen`. O CCL-G é o subsistema gráfico integrado ao CCL-C. O registro das decisões gráficas está em [ADR 0011](0011-nomenclatura-e-ccl-g.md) e [ADR 0012](0012-gummy-roadmap-de-backends.md).
- **Consequências:** O CCL-C permanece sem dependência de interface gráfica; a comunicação com um launcher futuro ocorrerá por processo.
- **Status:** Aceita.
