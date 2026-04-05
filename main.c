#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/pwm.h"
#include "menteviva.h"

app_t app;

static void pwm_init_rgb(void) {
    gpio_set_function(LED_R, GPIO_FUNC_PWM);
    gpio_set_function(LED_G, GPIO_FUNC_PWM);
    gpio_set_function(LED_B, GPIO_FUNC_PWM);
    pwm_set_wrap(pwm_gpio_to_slice_num(LED_R), 65535);
    pwm_set_wrap(pwm_gpio_to_slice_num(LED_G), 65535);
    pwm_set_wrap(pwm_gpio_to_slice_num(LED_B), 65535);
    pwm_set_enabled(pwm_gpio_to_slice_num(LED_R), true);
    pwm_set_enabled(pwm_gpio_to_slice_num(LED_G), true);
    pwm_set_enabled(pwm_gpio_to_slice_num(LED_B), true);
}

static uint32_t ultimo_mov = 0;

static void navegar_menu(void) {
    uint32_t agora = to_ms_since_boot(get_absolute_time());
    if (agora - ultimo_mov < 250) return;

    direcao_t dir = joy_direcao();
    if (dir == DIR_CIMA && app.cursor_menu > 0) {
        app.cursor_menu--;
        ultimo_mov = agora;
        buzzer_tom(1200, 30);
    } else if (dir == DIR_BAIXO && app.cursor_menu < 2) {
        app.cursor_menu++;
        ultimo_mov = agora;
        buzzer_tom(1200, 30);
    }
}

static void maquina_estados(void) {
    switch (app.tela) {

    case TELA_BOOT:
        app.tela = TELA_MENU;
        app.cursor_menu = 0;
        display_menu(0);
        matriz_limpar();
        rgb_set(0, 10, 0);
        printf("[menu] pronto\n");
        break;

    case TELA_MENU:
        navegar_menu();
        display_menu(app.cursor_menu);

        if (btn_a_apertou()) {
            const char *nomes[] = {"Memoria", "Reflexo", "Historico"};
            printf("[menu] selecionou: %s\n", nomes[app.cursor_menu]);
            buzzer_tom(880, 100);
            // TODO: implementar jogos nos proximos dias
        }
        break;
    }
}

int main(void) {
    stdio_init_all();
    sleep_ms(1000);

    printf("\n== MenteViva - Estimulador Cognitivo ==\n");
    printf("== BitDogLab v6.3 | EmbarcaTech      ==\n\n");

    pwm_init_rgb();
    rgb_set(20, 0, 20);

    input_init();
    printf("[init] input ok\n");

    buzzer_init();
    printf("[init] buzzer ok\n");

    matriz_init();
    printf("[init] matriz ok\n");

    display_init();
    display_boot();
    printf("[init] display ok\n");

    sleep_ms(2000);
    app.tela = TELA_BOOT;

    rgb_set(0, 10, 0);
    printf("[init] pronto!\n");

    buzzer_tom(660, 100);
    buzzer_tom(880, 100);
    buzzer_tom(1100, 200);

    while (true) {
        maquina_estados();
        sleep_ms(50);
    }
}
