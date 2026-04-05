#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "menteviva.h"

static uint32_t _fim_tom = 0;

static void _freq(uint pino, uint freq) {
    uint slice = pwm_gpio_to_slice_num(pino);

    if (freq == 0) {
        pwm_set_enabled(slice, false);
        return;
    }

    uint wrap = 1000000 / freq - 1;
    if (wrap < 1) wrap = 1;

    pwm_set_wrap(slice, wrap);
    pwm_set_chan_level(slice, pwm_gpio_to_channel(pino), wrap / 2);
    pwm_set_enabled(slice, true);
}

void buzzer_init(void) {
    gpio_set_function(BUZZER_A, GPIO_FUNC_PWM);
    gpio_set_function(BUZZER_B, GPIO_FUNC_PWM);
    pwm_set_clkdiv(pwm_gpio_to_slice_num(BUZZER_A), 125.0f);
    pwm_set_clkdiv(pwm_gpio_to_slice_num(BUZZER_B), 125.0f);
}

void buzzer_parar(void) {
    _freq(BUZZER_A, 0);
    _freq(BUZZER_B, 0);
    _fim_tom = 0;
}

void buzzer_tom(uint freq, uint duracao_ms) {
    _freq(BUZZER_A, freq);
    sleep_ms(duracao_ms);
    _freq(BUZZER_A, 0);
}

static void _tom_nb(uint freq, uint ms) {
    _freq(BUZZER_A, freq);
    _fim_tom = to_ms_since_boot(get_absolute_time()) + ms;
}

void buzzer_tick(void) {
    if (_fim_tom > 0 && to_ms_since_boot(get_absolute_time()) >= _fim_tom)
        buzzer_parar();
}

void buzzer_simon_tom(uint8_t quadrante) {
    static const uint tons[4] = {330, 440, 554, 659};
    if (quadrante < 4)
        _tom_nb(tons[quadrante], 180);
}