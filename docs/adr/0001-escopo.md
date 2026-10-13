# ADR 0001: Escopo

- **Contexto:** O projeto começa sem emulador e precisa evitar abstrações sem uso demonstrado.
- **Opções:** Criar desde já uma arquitetura genérica de múltiplos consoles; começar com um core concreto e abstrair após surgir uma segunda necessidade real.
- **Decisão:** Implementar primeiro um core GBA concreto. Compartilhar apenas o lado host necessário (configuração, caminhos de save, log, hash e probe). Abstrações comuns de core serão introduzidas quando um segundo core justificar isso.
- **Consequências:** O primeiro core pode refletir diretamente o hardware GBA; a arquitetura comum será baseada em necessidades observadas.
- **Status:** Aceita.
