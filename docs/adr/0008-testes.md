# ADR 0008: Testes

- **Contexto:** Testes de emulação precisam ser redistribuíveis e úteis sem incluir conteúdo protegido.
- **Opções:** Incluir ROMs proprietárias ou suítes de terceiros diretamente; criar testes próprios e avaliar dependências externas individualmente.
- **Decisão:** Criar ROMs de teste próprias sob GPL-3.0. Suítes de terceiros só entram após confirmação de licença, são baixadas por opção de CMake e nunca commitadas. Usar hash determinístico de framebuffer. Traces de emulador maduro podem servir como oráculo, sem copiar código.
- **Consequências:** Os testes do projeto permanecem auditáveis; dependências externas não são incorporadas sem revisão de licença.
- **Status:** Aceita.
