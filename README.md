# MenteViva

MenteViva é um projeto de **estimulação cognitiva interativa para idosos**, com foco em pessoas que podem apresentar sinais de **declínio cognitivo**, como em quadros relacionados ao **Alzheimer**, e também em pessoas com dificuldades de coordenação e tempo de resposta, como pode acontecer em condições associadas ao **Parkinson**.

Desenvolvido na **BitDogLab v6.3** com **Raspberry Pi Pico W / RP2040**, como projeto final do programa **EmbarcaTech**.

> **Aviso:** Este projeto tem finalidade educacional e experimental.
> Não substitui avaliação médica, diagnóstico ou tratamento.

## O que o projeto faz

O sistema oferece dois exercícios cognitivos e registra o desempenho ao longo do tempo.

### Jogo Simon (Memória)
Mostra uma sequência de quadrantes na matriz de LEDs. O usuário repete usando o joystick nas diagonais, acompanhando a posição dos cantos. A cada acerto, a sequência cresce. O score é o maior nível alcançado.

### Jogo Reflexo
Após um intervalo aleatório, um quadrante acende e o usuário aperta o botão A o mais rápido possível. São 5 rodadas por sessão, com penalidade por antecipação. O score é o tempo médio em milissegundos.

### Persistência e histórico
Os resultados ficam salvos na flash interna e sobrevivem a reinicializações. A tela de histórico mostra os últimos scores e uma indicação de tendência (melhorando, estável ou queda), calculada comparando as 3 sessões mais recentes com as 3 anteriores.

### Dashboard Wi-Fi
Quando conectado à rede, a placa serve uma página HTTP local com o último score de cada jogo e a tendência. O IP aparece no display OLED e na UART.

O sistema funciona normalmente mesmo sem Wi-Fi.

## Hardware

**Placa:** BitDogLab v6.3 — nenhum componente externo necessário.

| Periférico | GPIO | Função |
|---|---:|---|
| Botão A | 5 | Confirmar / resposta no Reflexo |
| Botão B | 6 | Voltar / sair |
| Joystick X/Y | 26 / 27 | Navegação e entrada no Simon |
| Botão do joystick | 22 | Reservado |
| OLED SDA/SCL | 14 / 15 | Display I2C |
| Matriz 5x5 WS2812B | 7 | Jogos visuais |
| Buzzer A | 21 | Feedback sonoro |
| Buzzer B | 10 | Reservado |
| LED RGB | 13 / 11 / 12 | Indicação de estado |
| Microfone | 28 | Seed para aleatoriedade |
| Wi-Fi CYW43439 | integrado | Dashboard HTTP |

## Como compilar

```bash
# configure o SSID e senha do Wi-Fi em wifi.c (defines WIFI_SSID e WIFI_PASSWORD)

mkdir build && cd build
cmake .. -DPICO_BOARD=pico_w
make -j4

# copie o menteviva.uf2 para a placa (BOOTSEL + USB)
```

**Requisitos:** Pico SDK 2.2.0, toolchain ARM GCC.

## Estrutura do projeto

```
MenteViva/
├── main.c           # máquina de estados, jogos, flash, tendência
├── menteviva.h      # defines e protótipos
├── input.c          # botões (IRQ), joystick (ADC), diagonal
├── display.c        # telas OLED com fonte 5x7
├── led_matrix.c     # matriz WS2812B, quadrantes
├── buzzer.c         # tons PWM
├── wifi.c           # conexão Wi-Fi e servidor HTTP
├── lwipopts.h       # configuração lwIP
├── CMakeLists.txt
├── docs/problemas/  # bugs resolvidos durante o desenvolvimento
└── lib/
    ├── neopixel_pio/ # driver WS2812B (BitDogLab-C)
    └── ssd1306/      # driver OLED (BitDogLab-C)
```

## Melhorias futuras

- Integração MQTT para acompanhamento remoto
- Sincronização de horário via NTP
- Dashboard mais completo com gráficos
- Heurísticas avançadas de análise cognitiva

## Dependências e créditos

- [Pico SDK 2.2.0](https://github.com/raspberrypi/pico-sdk) — SDK oficial do Raspberry Pi Pico
- [lwIP](https://savannah.nongnu.org/projects/lwip/) — pilha TCP/IP leve (incluída no Pico SDK)
- [BitDogLab-C](https://github.com/BitDogLab/BitDogLab-C) — drivers neopixel_pio e ssd1306 adaptados para o projeto
- Toolchain: ARM GCC 14.2 Rel1, CMake 3.31.5, Ninja

## Licença

MIT
