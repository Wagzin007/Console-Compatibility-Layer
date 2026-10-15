# ADR 0012: Roteiro de backends do Gummy

- **Contexto:** O CCL-G precisa de um roteiro de backends gráficos que preserve uma baseline compatível com hardware antigo.
- **Decisão aceita:** Usar Gummy OpenGL até consoles de complexidade aproximada de GameCube/Wii e PS2; a partir dessa classe, considerar Gummy Vulkan. A troca para Vulkan deve ocorrer por necessidade medida, não apenas pelo nome do console. OpenGL permanece como baseline para hardware antigo; Vulkan é adicional, salvo decisão futura em contrário.
- **Pendências propostas:** Definir critérios mensuráveis para a necessidade de Vulkan. A versão mínima de OpenGL permanece em aberto entre 3.3 core e 2.1, dependendo de `glxinfo` na máquina de teste. Não desenhar uma interface de backend pensando em Vulkan antes de existir o segundo backend.
- **Referência técnica:** A compatibilidade de OpenGL depende do suporte de driver e do hardware; consultar a especificação de cada backend durante sua implementação. Nenhuma dependência ou interface de backend é introduzida nesta decisão.
- **Status:** Aceita para o roteiro. Critério de necessidade e versão mínima de OpenGL permanecem pendentes.
