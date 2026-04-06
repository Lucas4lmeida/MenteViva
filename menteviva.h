#ifndef MENTEVIVA_H
#define MENTEVIVA_H

#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"

// pinos BitDogLab v6.3
#define BTN_A           5
#define BTN_B           6
#define JOY_X           26
#define JOY_Y           27
#define JOY_BTN         22
#define OLED_SDA        14
#define OLED_SCL        15
#define LED_MATRIX_PIN  7
#define BUZZER_A        21
#define BUZZER_B        10
#define LED_R           13
#define LED_G           11
#define LED_B           12
#define MIC_PIN         28

// oled
#define OLED_W          128
#define OLED_H          64
#define OLED_ADDR       0x3C
#define OLED_I2C        i2c1

// matriz
#define NUM_LEDS        25

// joystick
#define JOY_CENTRO      2048
#define JOY_MARGEM      600

typedef enum {
    DIR_NENHUM = 0,
    DIR_CIMA,
    DIR_BAIXO,
    DIR_ESQUERDA,
    DIR_DIREITA
} direcao_t;

typedef enum {
    TELA_BOOT,
    TELA_MENU,
    TELA_SIMON,
    TELA_REFLEXO
} tela_t;

typedef struct {
    tela_t  tela;
    uint8_t cursor_menu;
} app_t;

extern app_t app;

// input.c
void      input_init(void);
bool      btn_a_apertou(void);
bool      btn_b_apertou(void);
bool      joy_btn_apertou(void);
direcao_t joy_direcao(void);

// display.c
void display_init(void);
void display_boot(void);
void display_menu(uint8_t cursor);

void display_simon_observe(uint8_t nivel);
void display_simon_jogue(uint8_t nivel, uint8_t passo);
void display_simon_resultado(const char *linha1, const char *linha2);

void display_reflexo_aguarde(uint8_t rodada);
void display_reflexo_pronto(uint8_t rodada);
void display_reflexo_parcial(uint8_t rodada, bool antecipou, uint32_t tempo_ms);
void display_reflexo_final(uint32_t media_ms, uint8_t antecipacoes);

// buzzer.c
void buzzer_init(void);
void buzzer_tom(uint freq, uint duracao_ms);
void buzzer_simon_tom(uint8_t quadrante);
void buzzer_parar(void);
void buzzer_tick(void);

// led_matrix.c
void matriz_init(void);
void matriz_limpar(void);
void matriz_quadrante(uint8_t q, uint8_t r, uint8_t g, uint8_t b);
void matriz_quadrante_padrao(uint8_t q);
void matriz_todos_fracos(void);

// led rgb
static inline void rgb_set(uint8_t r, uint8_t g, uint8_t b) {
    pwm_set_gpio_level(LED_R, r * r);
    pwm_set_gpio_level(LED_G, g * g);
    pwm_set_gpio_level(LED_B, b * b);
}

#endif