# ADR 0010: Validação de ROM

- **Contexto:** O comando `info` precisa identificar metadados GBA e informar problemas sem rejeitar homebrew, hacks ou dumps incomuns.
- **Opções:** Tratar todo campo inesperado como erro; separar falhas estruturais de avisos de integridade e formato.
- **Decisão:** Arquivo vazio, cabeçalho menor que 192 bytes ou ROM acima de 32 MiB é erro. Byte fixo diferente de `0x96`, checksum divergente, código da unidade diferente de `0x00`, título com bytes não ASCII imprimíveis e tamanho não múltiplo de quatro geram avisos. ROM desconhecida ou com avisos não é inválida por esse motivo.
- **Consequências:** `ccl-run info` pode apresentar metadados de homebrew, hacks e dumps incomuns sem prometer que sejam executáveis. A leitura é somente leitura e limitada a 32 MiB.
- **Logo:** O campo de logo não é validado nem embutido. A validação feita pela BIOS real exigiria dados proprietários; boot direto não depende dela.
- **Status:** Aceita.
