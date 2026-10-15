# ADR 0005: Erros

- **Contexto:** Erros de infraestrutura e comportamentos indefinidos do guest exigem tratamentos diferentes.
- **Opções:** Exceções em todo o código; códigos globais de erro; erros de host na borda e tratamento explícito de comportamento do guest.
- **Decisão:** Erros de host (arquivo ou configuração) retornam `Result` na borda. Acessos do guest não suportados pelo bus geram eventos consultáveis, sem stdout. Os eventos são deduplicados por tipo e endereço alinhado, limitados a 256 entradas e contam eventos adicionais descartados. Uma flag `strict` fica consultável para que testes falhem quando houver eventos. Exceções não são usadas no caminho quente.
- **Consequências:** Falhas de host são reportáveis ao chamador e casos do guest podem seguir a emulação sem exceções frequentes; um fluxo grande de eventos não consome memória sem limite.
- **Status:** Aceita.
