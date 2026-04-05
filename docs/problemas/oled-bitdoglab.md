# Problema resolvido: OLED com ruído na BitDogLab

## Sintoma
O display OLED ligava, mas mostrava apenas ruído, faixa horizontal ou imagem sem interface válida.

## Ambiente
- Projeto: MenteViva
- Placa: BitDogLab v6.3
- OLED: SSD1306
- I2C usado: `i2c1`
- SDA/SCL: `GPIO 14` e `GPIO 15`
- Endereço: `0x3C`

## Causa
A biblioteca `lib/ssd1306/inc/ssd1306.c` enviava comandos do OLED em escritas I2C muito curtas.
Na prática, a inicialização ficava parcial e o display energizava sem configurar corretamente.

## Correção aplicada
Foi criada uma função para enviar comandos em bloco:

```c
static void ssd1306_write_cmds(ssd1306_t *ssd, const uint8_t *cmds, size_t count)
```

Depois disso:
- a inicialização passou a funcionar
- `ssd1306_fill()` e `ssd1306_send_data()` passaram a responder
- o menu e os textos apareceram normalmente

## Configuração confirmada
```c
#define OLED_I2C  i2c1
#define OLED_SDA  14
#define OLED_SCL  15
#define OLED_ADDR 0x3C
```

## Arquivos envolvidos
- `display.c`
- `lib/ssd1306/inc/ssd1306.c`

## Observação
Se o OLED voltar a mostrar ruído ou faixa horizontal, verificar primeiro:
1. se o projeto continua usando `i2c1`
2. se SDA/SCL estão em `14/15`
3. se o endereço continua `0x3C`
4. se a versão corrigida de `ssd1306.c` ainda está no projeto

## Status
Resolvido.
