#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "menteviva.h"

void buzzer_init(void) {
    gpio_set_function(BUZZER_A, GPIO_FUNC_PWM);
    gpio_set_function(BUZZER_B, GPIO_FUNC_PWM);
    pwm_set_clkdiv(pwm_gpio_to_slice_num(BUZZER_A), 125.0f);
    pwm_set_clkdiv(pwm_gpio_to_slice_num(BUZZER_B), 125.0f);
}

void buzzer_tom(uint freq, uint duracao_ms) {
    uint slice = pwm_gpio_to_slice_num(BUZZER_A);
    if (freq == 0) { pwm_set_enabled(slice, false); return; }
    uint wrap = 1000000 / freq - 1;
    if (wrap < 1) wrap = 1;
    pwm_set_wrap(slice, wrap);
    pwm_set_chan_level(slice, pwm_gpio_to_channel(BUZZER_A), wrap / 2);
    pwm_set_enabled(slice, true);
    sleep_ms(duracao_ms);
    pwm_set_enabled(slice, false);
}
