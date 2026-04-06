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
static volatile uint32_t _a_press_us = 0;

#define DEBOUNCE 200

// Leitura diagonal do Simon:
// - zona morta menor para não exigir canto "perfeito"
// - eixo mínimo para evitar falso positivo
// - relação máxima entre eixos para rejeitar movimento quase cardinal
#define DIAG_ZONA_MORTA 220
#define DIAG_MIN_EIXO   280
#define DIAG_MIN_SOMA   950

static void gpio_callback(uint gpio, uint32_t events) {
    if (!(events & GPIO_IRQ_EDGE_FALL)) return;
    uint32_t agora = to_ms_since_boot(get_absolute_time());

    if (gpio == BTN_A && agora - _a_tempo > DEBOUNCE) {
        _a_flag = true;
        _a_tempo = agora;
        _a_press_us = time_us_32();
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

static void joy_ler_xy(int *x, int *y) {
    // Mantém o mesmo mapeamento lógico já validado no projeto:
    // adc0 entra como eixo vertical lógico
    // adc1 entra como eixo horizontal lógico
    adc_select_input(0);
    *y = (int)adc_read() - JOY_CENTRO;

    adc_select_input(1);
    *x = (int)adc_read() - JOY_CENTRO;
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
    if (_a_flag) {
        _a_flag = false;
        return true;
    }
    return false;
}

bool btn_b_apertou(void) {
    if (_b_flag) {
        _b_flag = false;
        return true;
    }
    return false;
}

bool joy_btn_apertou(void) {
    if (_j_flag) {
        _j_flag = false;
        return true;
    }
    return false;
}

uint32_t btn_a_tempo_us(void) {
    return _a_press_us;
}

direcao_t joy_direcao(void) {
    int x, y;
    joy_ler_xy(&x, &y);

    int ax = x < 0 ? -x : x;
    int ay = y < 0 ? -y : y;

    if (ax < (int)JOY_MARGEM && ay < (int)JOY_MARGEM)
        return DIR_NENHUM;

    if (ax > ay)
        return x > 0 ? DIR_DIREITA : DIR_ESQUERDA;
    else
        return y > 0 ? DIR_CIMA : DIR_BAIXO;
}

int8_t joy_quadrante_diagonal(void) {
    int x, y;
    joy_ler_xy(&x, &y);

    int ax = x < 0 ? -x : x;
    int ay = y < 0 ? -y : y;

    // zona morta pequena para não exigir perfeição
    if (ax < DIAG_ZONA_MORTA && ay < DIAG_ZONA_MORTA)
        return -1;

    // precisa haver componente clara nos dois eixos
    if (ax < DIAG_MIN_EIXO || ay < DIAG_MIN_EIXO)
        return -1;

    // precisa ser um movimento suficientemente "de canto"
    if ((ax + ay) < DIAG_MIN_SOMA)
        return -1;

    // rejeita movimento quase puro horizontal/vertical
    {
        int maior = ax > ay ? ax : ay;
        int menor = ax > ay ? ay : ax;

        // aceita diagonais bem abertas, mas não qualquer coisa
        if (menor * 4 < maior)
            return -1;
    }

    // quadrantes do Simon:
    // q0 = superior esquerdo
    // q1 = superior direito
    // q2 = inferior direito
    // q3 = inferior esquerdo
    if (x < 0 && y > 0) return 0;
    if (x > 0 && y > 0) return 1;
    if (x > 0 && y < 0) return 2;
    if (x < 0 && y < 0) return 3;

    return -1;
}