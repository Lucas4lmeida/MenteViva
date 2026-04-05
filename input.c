#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "menteviva.h"

static volatile bool _a_flag = false;
static volatile bool _b_flag = false;
static volatile bool _j_flag = false;
static volatile uint32_t _a_tempo = 0;
static volatile uint32_t _b_tempo = 0;
static volatile uint32_t _j_tempo = 0;

#define DEBOUNCE 200

static void gpio_callback(uint gpio, uint32_t events) {
    if (!(events & GPIO_IRQ_EDGE_FALL)) return;
    uint32_t agora = to_ms_since_boot(get_absolute_time());

    if (gpio == BTN_A && agora - _a_tempo > DEBOUNCE) {
        _a_flag = true;
        _a_tempo = agora;
    }
    if (gpio == BTN_B && agora - _b_tempo > DEBOUNCE) {
        _b_flag = true;
        _b_tempo = agora;
    }
    if (gpio == JOY_BTN && agora - _j_tempo > DEBOUNCE) {
        _j_flag = true;
        _j_tempo = agora;
    }
}

void input_init(void) {
    gpio_init(BTN_A);
    gpio_set_dir(BTN_A, GPIO_IN);
    gpio_pull_up(BTN_A);

    gpio_init(BTN_B);
    gpio_set_dir(BTN_B, GPIO_IN);
    gpio_pull_up(BTN_B);

    gpio_init(JOY_BTN);
    gpio_set_dir(JOY_BTN, GPIO_IN);
    gpio_pull_up(JOY_BTN);

    gpio_set_irq_enabled_with_callback(BTN_A, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    gpio_set_irq_enabled(BTN_B, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(JOY_BTN, GPIO_IRQ_EDGE_FALL, true);

    adc_init();
    adc_gpio_init(JOY_X);
    adc_gpio_init(JOY_Y);
}

bool btn_a_apertou(void) {
    if (_a_flag) { _a_flag = false; return true; }
    return false;
}

bool btn_b_apertou(void) {
    if (_b_flag) { _b_flag = false; return true; }
    return false;
}

bool joy_btn_apertou(void) {
    if (_j_flag) { _j_flag = false; return true; }
    return false;
}

direcao_t joy_direcao(void) {
    adc_select_input(0);
    int x = (int)adc_read() - JOY_CENTRO;
    adc_select_input(1);
    int y = (int)adc_read() - JOY_CENTRO;

    int ax = x < 0 ? -x : x;
    int ay = y < 0 ? -y : y;

    if (ax < (int)JOY_MARGEM && ay < (int)JOY_MARGEM)
        return DIR_NENHUM;

    if (ax > ay)
        return x > 0 ? DIR_DIREITA : DIR_ESQUERDA;
    else
        return y > 0 ? DIR_BAIXO : DIR_CIMA;
}
