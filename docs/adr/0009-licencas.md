# ADR 0009: Licenças

- **Contexto:** O projeto já está sob GPL-3.0 e precisa registrar a compatibilidade das dependências e de eventual código reutilizado.
- **Opções:** Manter GPL-3.0 e auditar contribuições; alterar a licença do projeto.
- **Decisão:** Manter GPL-3.0 e registrar cada dependência em `NOTICE`. Código GPL-2.0-only é incompatível com a licença do projeto; código MPL-2.0 exige preservar os avisos aplicáveis.
- **Consequências:** Dependências e contribuições precisam de revisão de licença. A questão sobre eventual reaproveitamento de código de outros emuladores permanece pendente.
- **Status:** Proposta; pendente de decisão sobre reaproveitamento de código de outros emuladores.
