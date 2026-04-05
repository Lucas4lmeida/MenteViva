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
static uint32_t ultimo_preview = 0;
static int8_t ultimo_quadrante = -1;

static int8_t direcao_para_quadrante(direcao_t dir) {
    switch (dir) {
        case DIR_CIMA:     return 0;
        case DIR_DIREITA:  return 1;
        case DIR_BAIXO:    return 2;
        case DIR_ESQUERDA: return 3;
        default:           return -1;
    }
}

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

static void entrar_teste_memoria(void) {
    app.tela = TELA_TESTE_MEMORIA;
    ultimo_preview = 0;
    ultimo_quadrante = -1;

    display_teste_memoria(DIR_NENHUM, -1);
    matriz_todos_fracos();
    rgb_set(0, 0, 20);

    printf("[teste] memoria pronto\n");
}

static void voltar_menu(void) {
    app.tela = TELA_MENU;
    display_menu(app.cursor_menu);
    matriz_limpar();
    rgb_set(0, 10, 0);
}

static void tick_teste_memoria(void) {
    uint32_t agora = to_ms_since_boot(get_absolute_time());
    direcao_t dir = joy_direcao();
    int8_t q = direcao_para_quadrante(dir);

    if (btn_b_apertou()) {
        printf("[teste] memoria encerrado\n");
        buzzer_parar();
        voltar_menu();
        return;
    }

    display_teste_memoria(dir, q);

    if (q >= 0) {
        if (q != ultimo_quadrante || agora - ultimo_preview > 250) {
            ultimo_quadrante = q;
            ultimo_preview = agora;

            matriz_quadrante_padrao(q);
            buzzer_simon_tom(q);

            printf("[teste] dir=%d q=%d\n", dir, q);
        }
    } else if (ultimo_quadrante != -1) {
        ultimo_quadrante = -1;
        matriz_todos_fracos();
        printf("[teste] dir=0 q=-1\n");
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

            if (app.cursor_menu == 0) {
                entrar_teste_memoria();
            } else {
                buzzer_tom(880, 100);
                // TODO: implementar proximas telas nos proximos dias
            }
        }
        break;

    case TELA_TESTE_MEMORIA:
        tick_teste_memoria();
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
        buzzer_tick();
        maquina_estados();
        sleep_ms(50);
    }
}