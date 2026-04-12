# Problema resolvido: OLED com ruído na BitDogLab

## Sintoma

Ao ligar o projeto na BitDogLab v6.3, o display OLED acendia, mas não mostrava a interface corretamente. Em vez disso, aparecia ruído na tela.

Em alguns testes, o display parecia responder, mas o conteúdo desenhado não aparecia de forma correta.

## Contexto

Projeto: MenteViva  
Placa: BitDogLab v6.3  
MCU: Raspberry Pi Pico W / RP2040  
Biblioteca usada: `lib/ssd1306/inc/ssd1306.c`

## Verificações feitas

A primeira suspeita foi pinagem ou barramento I2C, mas os testes mostraram que a configuração usada estava correta:

- I2C: `i2c1`
- SDA: GPIO 14
- SCL: GPIO 15
- endereço: `0x3C`

Também foi verificado que o problema não estava na fonte 5x7 implementada no `display.c`, nem na lógica do menu.

## Causa

O problema estava na forma como a biblioteca `ssd1306.c` enviava os comandos de inicialização do display.

Os comandos estavam sendo enviados em escritas I2C muito curtas, o que fez o OLED não inicializar corretamente na prática. Com isso, o display ligava, mas não era configurado como deveria.

## Solução aplicada

Foi alterado o arquivo:

```text
lib/ssd1306/inc/ssd1306.c