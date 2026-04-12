# MenteViva

MenteViva é um projeto de **estimulação cognitiva interativa para idosos**, com foco especial em pessoas que podem apresentar sinais de **declínio cognitivo**, como em quadros relacionados ao **Alzheimer**, e também em pessoas com dificuldades de coordenação e tempo de resposta, como pode acontecer em condições associadas ao **Parkinson**.

O projeto foi desenvolvido na **BitDogLab v6.3** com **Raspberry Pi Pico W / RP2040**, como projeto final do programa **EmbarcaTech**.

A proposta é reunir atividades simples, visuais e acessíveis para estimular memória, atenção e tempo de resposta, usando recursos embarcados da placa e um dashboard local via Wi-Fi.

> Este projeto tem finalidade **educacional e experimental**.  
> Ele **não substitui avaliação médica, diagnóstico, acompanhamento clínico ou tratamento**.

## Objetivo

O objetivo do MenteViva é desenvolver um sistema embarcado simples e acessível que ofereça exercícios interativos voltados ao estímulo cognitivo e motor, principalmente para o público idoso.

A ideia principal é explorar:

- memória visual
- atenção
- tempo de reação
- interação com feedback sonoro e visual
- acompanhamento simples de desempenho

## Público de interesse

O projeto foi pensado principalmente para:

- idosos em geral
- pessoas com redução de atenção ou memória
- pessoas com dificuldades motoras leves
- contextos de interesse ligados a **Alzheimer** e **Parkinson**

No caso do Alzheimer, a proposta se relaciona mais com **memória e sequência**.  
No caso do Parkinson, a proposta se relaciona mais com **resposta motora e tempo de reação**.

## Funcionalidades implementadas

O sistema possui duas atividades principais.

### 1. Jogo Simon (Memória)
O sistema mostra uma sequência de quadrantes na matriz de LEDs e o usuário deve repetir essa sequência usando o joystick.

Características:
- sequência progressiva
- interação visual com os quadrantes
- entrada por diagonais no joystick, acompanhando a posição dos cantos da matriz
- níveis crescentes
- score baseado no maior nível alcançado

### 2. Jogo Reflexo
O sistema espera um intervalo aleatório, acende um quadrante e o usuário deve apertar o botão **A** o mais rápido possível.

Características:
- 5 rodadas por partida
- penalidade por antecipação
- cálculo de tempo médio em milissegundos
- score baseado no desempenho da sessão

## Recursos extras implementados

Além dos jogos, o projeto também possui:

- persistência de scores na flash interna
- histórico de sessões
- dashboard HTTP local via Wi-Fi
- logs pela UART para depuração

## Análise de desempenho

Atualmente, o projeto registra:

- score do Simon
- média do Reflexo
- histórico simples das últimas sessões

A parte de **heurísticas mais elaboradas de análise dos resultados** fica como **evolução futura** do projeto.

Ou seja, embora o sistema já armazene dados de desempenho, uma interpretação mais inteligente desses resultados ainda pode ser expandida no futuro, por exemplo com:

- análise de progresso por período
- comparação de sessões com critérios mais robustos
- alertas simples de piora ou melhora
- visualizações mais completas no dashboard

## Hardware utilizado

Placa principal: **BitDogLab v6.3**

### Periféricos usados

| Periférico | GPIO | Função |
|---|---:|---|
| Botão A | 5 | Ação principal / resposta no Reflexo |
| Botão B | 6 | Voltar / sair |
| Joystick X | 26 | Entrada analógica |
| Joystick Y | 27 | Entrada analógica |
| Botão do joystick | 22 | Reservado |
| OLED SDA | 14 | I2C |
| OLED SCL | 15 | I2C |
| Matriz 5x5 WS2812B | 7 | Saída visual |
| Buzzer A | 21 | Feedback sonoro |
| Buzzer B | 10 | Reservado |
| LED RGB | 13 / 11 / 12 | Status do sistema |
| Microfone | 28 | Seed simples para aleatoriedade |
| Wi-Fi CYW43439 | embutido | Dashboard local |

## Como o sistema funciona

### Fluxo geral
1. A placa inicializa os periféricos
2. Tenta conectar ao Wi-Fi
3. Mostra o status da conexão no display
4. Entra no menu principal
5. O usuário escolhe entre:
   - Memória
   - Reflexo
   - Histórico

## Dashboard Wi-Fi

Quando a placa conecta à rede, ela sobe um servidor HTTP local.

No dashboard é possível visualizar:

- último score do Simon
- último resultado do Reflexo
- informações simples de acompanhamento

O acesso é feito pelo IP mostrado no display e também pela UART.

## Estrutura do projeto

```text
MenteViva/
├── main.c
├── menteviva.h
├── input.c
├── display.c
├── led_matrix.c
├── buzzer.c
├── wifi.c
├── lwipopts.h
├── CMakeLists.txt
├── pico_sdk_import.cmake
├── docs/
│   └── problemas/
├── lib/
│   ├── neopixel_pio/
│   └── ssd1306/
└── build/
