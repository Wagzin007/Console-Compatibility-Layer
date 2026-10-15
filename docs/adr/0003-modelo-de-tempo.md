# ADR 0003: Modelo de tempo

- **Contexto:** A execução do GBA pelo módulo correspondente do CCL-C precisa de uma base determinística de ciclos para sincronizar CPU e hardware.
- **Opções:** Avançar componentes por tempo de parede; usar eventos relativos; usar contador absoluto de ciclos.
- **Decisão:** Usar `Scheduler` concreto, sem callbacks virtuais ou `std::function`, com contador absoluto de ciclos `std::uint64_t`. Eventos são enums com payload simples. `schedule(at, event)` devolve handle; também há `cancel(handle)`, `next_deadline()` e `pop_due(now)`. Eventos simultâneos preservam FIFO de inserção; agendamento no passado e regressão do relógio abortam explicitamente. Um chamador pode reagendar após consumir cada evento de `pop_due`. O frame GBA tem 280.896 ciclos: 1.232 ciclos por scanline e 228 scanlines.
- **Fonte de temporização:** GBATEK, seção “LCD Dimensions and Timings” (1.4.18), https://problemkaputt.de/gbatek.htm.
- **Consequências:** Componentes do CCL-C podem sincronizar-se por um relógio determinístico; a futura PPU do CCL-G poderá avançar por scanline e ser refinada conforme a precisão necessária.
- **Status:** Aceita.
