# ADR 0003: Modelo de tempo

- **Contexto:** A execução do GBA precisa de uma base determinística de ciclos para sincronizar CPU e hardware.
- **Opções:** Avançar componentes por tempo de parede; usar eventos relativos; usar contador absoluto de ciclos.
- **Decisão:** Usar scheduler de eventos com contador absoluto de ciclos `u64` e CPU com `run_until(ciclo)`. Um frame GBA corresponde a 280.896 ciclos. Inicialmente, a PPU avança por scanline.
- **Consequências:** Componentes podem sincronizar-se por um relógio determinístico; a PPU poderá ser refinada conforme a precisão necessária.
- **Status:** Aceita.
