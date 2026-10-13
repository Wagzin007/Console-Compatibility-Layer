# ADR 0005: Erros

- **Contexto:** Erros de infraestrutura e comportamentos indefinidos do guest exigem tratamentos diferentes.
- **Opções:** Exceções em todo o código; códigos globais de erro; erros de host na borda e tratamento explícito de comportamento do guest.
- **Decisão:** Erros de host (arquivo ou configuração) retornam `Result` na borda. Comportamento indefinido do guest é registrado uma vez e recebe comportamento de hardware; `--strict` falha. Exceções não são usadas no caminho quente.
- **Consequências:** Falhas de host são reportáveis ao chamador e casos do guest podem seguir a emulação sem exceções frequentes.
- **Status:** Aceita.
