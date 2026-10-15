# ADR 0001: Escopo

- **Contexto:** O projeto começa sem emulador e precisa evitar abstrações sem uso demonstrado.
- **Opções:** Criar desde já uma arquitetura genérica de múltiplos consoles; começar com o módulo GBA do CCL-C e abstrair após surgir uma segunda necessidade real.
- **Decisão:** Implementar primeiro o módulo GBA do CCL-C. Compartilhar apenas o lado host necessário (configuração, caminhos de save, log, hash e probe). Abstrações comuns para consoles serão introduzidas quando um segundo console justificar isso.
- **Consequências:** O CCL-C pode refletir diretamente o hardware GBA; a arquitetura comum será baseada em necessidades observadas.
- **Status:** Aceita.
