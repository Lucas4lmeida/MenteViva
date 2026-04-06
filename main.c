#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/pwm.h"
#include "menteviva.h"

app_t app;

static const uint8_t simon_base[3] = {0, 1, 2};

static uint8_t simon_nivel = 1;
static uint8_t simon_indice = 0;
static bool simon_joy_livre = false;
static uint32_t ultimo_mov = 0;

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

static void tocar_quadrante(uint8_t q, uint16_t tempo_ms) {
    matriz_quadrante_padrao(q);
    buzzer_simon_tom(q);
    sleep_ms(tempo_ms);
    buzzer_parar();
    matriz_limpar();
}

static void voltar_menu(void) {
    app.tela = TELA_MENU;
    display_menu(app.cursor_menu);
    matriz_limpar();
    rgb_set(0, 10, 0);
}

static void simon_mostrar_nivel(void) {
    display_simon_observe(simon_nivel);
    rgb_set(0, 0, 20);
    matriz_limpar();
    sleep_ms(400);

    for (uint8_t i = 0; i < simon_nivel; i++) {
        tocar_quadrante(simon_base[i], 180);
        sleep_ms(140);
    }

    simon_indice = 0;
    display_simon_jogue(simon_nivel, 1);
    matriz_todos_fracos();

    while (joy_direcao() != DIR_NENHUM) {
        sleep_ms(20);
    }

    simon_joy_livre = true;
}

static void simon_iniciar(void) {
    app.tela = TELA_SIMON;
    simon_nivel = 1;
    simon_indice = 0;
    simon_joy_livre = false;

    printf("[simon] iniciar\n");
    simon_mostrar_nivel();
}

static void simon_erro(void) {
    printf("[simon] erro no nivel %d\n", simon_nivel);

    rgb_set(20, 0, 0);
    display_simon_resultado("Errou a", "sequencia");
    buzzer_tom(220, 300);
    sleep_ms(900);

    voltar_menu();
}

static void simon_proximo_nivel_ou_fim(void) {
    if (simon_nivel >= 3) {
        printf("[simon] concluiu nivel 3\n");

        rgb_set(0, 20, 0);
        display_simon_resultado("Parabens!", "Nivel 3 ok");
        buzzer_tom(880, 80);
        buzzer_tom(1100, 100);
        buzzer_tom(1320, 120);
        sleep_ms(1000);

        voltar_menu();
        return;
    }

    simon_nivel++;
    printf("[simon] avancou para nivel %d\n", simon_nivel);

    rgb_set(0, 20, 0);
    display_simon_resultado("Acertou!", "Prox nivel");
    buzzer_tom(880, 80);
    buzzer_tom(1100, 120);
    sleep_ms(600);

    simon_mostrar_nivel();
}

static void tick_simon(void) {
    if (btn_b_apertou()) {
        printf("[simon] saiu pelo botao B\n");
        display_simon_resultado("Saindo...", "Voltando menu");
        sleep_ms(500);
        voltar_menu();
        return;
    }

    direcao_t dir = joy_direcao();

    if (dir == DIR_NENHUM) {
        simon_joy_livre = true;
        return;
    }

    if (!simon_joy_livre) return;

    simon_joy_livre = false;

    int8_t q = direcao_para_quadrante(dir);
    if (q < 0) return;

    printf("[simon] entrada dir=%d q=%d passo=%d\n", dir, q, simon_indice + 1);

    tocar_quadrante((uint8_t)q, 120);

    if ((uint8_t)q != simon_base[simon_indice]) {
        simon_erro();
        return;
    }

    simon_indice++;

    if (simon_indice >= simon_nivel) {
        simon_proximo_nivel_ou_fim();
        return;
    }

    display_simon_jogue(simon_nivel, simon_indice + 1);
    matriz_todos_fracos();
    rgb_set(0, 0, 20);
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
                simon_iniciar();
            } else {
                buzzer_tom(880, 100);
                // TODO: implementar proximas telas nos proximos dias
            }
        }
        break;

    case TELA_SIMON:
        tick_simon();
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