#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "menteviva.h"

app_t app;

#define SIMON_MAX_NIVEIS 5

typedef enum {
    REFLEXO_ESPERANDO = 0,
    REFLEXO_PRONTO,
    REFLEXO_RESULTADO
} reflexo_estado_t;

static uint8_t simon_seq[SIMON_MAX_NIVEIS];
static uint8_t simon_nivel = 1;
static uint8_t simon_indice = 0;
static bool simon_joy_livre = false;

static reflexo_estado_t reflexo_estado = REFLEXO_ESPERANDO;
static uint8_t reflexo_q = 0;
static uint32_t reflexo_deadline = 0;
static absolute_time_t reflexo_inicio;

static uint32_t ultimo_mov = 0;

static uint32_t agora_ms(void) {
    return to_ms_since_boot(get_absolute_time());
}

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

static void rng_init(void) {
    uint32_t seed = time_us_32();

    adc_gpio_init(MIC_PIN);

    for (int i = 0; i < 32; i++) {
        adc_select_input(2);
        sleep_us(100);
        seed = (seed << 1) ^ adc_read() ^ time_us_32();
    }

    srand(seed);
    printf("[rng] seed ok\n");
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
    uint32_t agora = agora_ms();
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

static void simon_gerar_sequencia(void) {
    for (uint8_t i = 0; i < SIMON_MAX_NIVEIS; i++) {
        simon_seq[i] = rand() % 4;
    }
}

static bool simon_mostrar_nivel(void) {
    display_simon_observe(simon_nivel);
    rgb_set(0, 0, 20);
    matriz_limpar();
    sleep_ms(350);

    for (uint8_t i = 0; i < simon_nivel; i++) {
        if (btn_b_apertou()) {
            printf("[simon] saiu durante exibicao\n");
            voltar_menu();
            return false;
        }

        tocar_quadrante(simon_seq[i], 180);
        sleep_ms(140);
    }

    simon_indice = 0;
    display_simon_jogue(simon_nivel, 1);
    matriz_todos_fracos();

    while (joy_direcao() != DIR_NENHUM) {
        if (btn_b_apertou()) {
            printf("[simon] saiu aguardando neutro\n");
            voltar_menu();
            return false;
        }
        sleep_ms(20);
    }

    simon_joy_livre = true;
    return true;
}

static void simon_iniciar(void) {
    app.tela = TELA_SIMON;
    simon_nivel = 1;
    simon_indice = 0;
    simon_joy_livre = false;

    simon_gerar_sequencia();

    printf("[simon] iniciar\n");
    simon_mostrar_nivel();
}

static void simon_erro(void) {
    char buf[24];
    uint8_t score = (simon_nivel > 0) ? (simon_nivel - 1) : 0;

    printf("[simon] erro no nivel %d\n", simon_nivel);

    rgb_set(20, 0, 0);
    snprintf(buf, sizeof(buf), "Score: %d", score);
    display_simon_resultado("Errou!", buf);

    buzzer_tom(220, 300);
    sleep_ms(900);

    voltar_menu();
}

static void simon_vitoria(void) {
    char buf[24];

    printf("[simon] concluiu nivel %d\n", SIMON_MAX_NIVEIS);

    rgb_set(0, 20, 0);
    snprintf(buf, sizeof(buf), "Score: %d", SIMON_MAX_NIVEIS);
    display_simon_resultado("Parabens!", buf);

    buzzer_tom(880, 80);
    buzzer_tom(1100, 100);
    buzzer_tom(1320, 120);
    sleep_ms(1000);

    voltar_menu();
}

static void simon_proximo_nivel(void) {
    simon_nivel++;

    if (simon_nivel > SIMON_MAX_NIVEIS) {
        simon_vitoria();
        return;
    }

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

    printf("[simon] entrada q=%d passo=%d\n", q, simon_indice + 1);

    tocar_quadrante((uint8_t)q, 120);

    if ((uint8_t)q != simon_seq[simon_indice]) {
        simon_erro();
        return;
    }

    simon_indice++;

    if (simon_indice >= simon_nivel) {
        if (simon_nivel >= SIMON_MAX_NIVEIS) {
            simon_vitoria();
        } else {
            simon_proximo_nivel();
        }
        return;
    }

    display_simon_jogue(simon_nivel, simon_indice + 1);
    matriz_todos_fracos();
    rgb_set(0, 0, 20);
}

static void reflexo_nova_rodada(void) {
    reflexo_q = rand() % 4;
    reflexo_deadline = agora_ms() + 1200 + (rand() % 1800);
    reflexo_estado = REFLEXO_ESPERANDO;

    matriz_limpar();
    rgb_set(10, 0, 10);
    display_reflexo_aguarde();

    printf("[reflexo] nova rodada q=%d\n", reflexo_q);
}

static void reflexo_iniciar(void) {
    app.tela = TELA_REFLEXO;
    reflexo_nova_rodada();
}

static void reflexo_resultado_tempo(uint32_t tempo_ms) {
    char buf[24];

    matriz_limpar();
    rgb_set(0, 20, 0);

    snprintf(buf, sizeof(buf), "Tempo: %lu ms", tempo_ms);
    display_reflexo_resultado("Boa!", buf);

    buzzer_tom(1000, 80);
    reflexo_estado = REFLEXO_RESULTADO;

    printf("[reflexo] tempo=%lu ms\n", tempo_ms);
}

static void reflexo_resultado_antecipou(void) {
    matriz_limpar();
    rgb_set(20, 0, 0);
    display_reflexo_resultado("Muito cedo!", "Tente de novo");

    buzzer_tom(220, 250);
    reflexo_estado = REFLEXO_RESULTADO;

    printf("[reflexo] antecipou\n");
}

static void tick_reflexo(void) {
    if (btn_b_apertou()) {
        printf("[reflexo] saiu pelo botao B\n");
        voltar_menu();
        return;
    }

    switch (reflexo_estado) {

    case REFLEXO_ESPERANDO:
        if (btn_a_apertou()) {
            reflexo_resultado_antecipou();
            return;
        }

        if (agora_ms() >= reflexo_deadline) {
            reflexo_estado = REFLEXO_PRONTO;
            reflexo_inicio = get_absolute_time();

            matriz_quadrante_padrao(reflexo_q);
            display_reflexo_pronto();
            buzzer_simon_tom(reflexo_q);
            rgb_set(0, 0, 20);

            printf("[reflexo] alvo liberado q=%d\n", reflexo_q);
        }
        break;

    case REFLEXO_PRONTO:
        if (btn_a_apertou()) {
            uint32_t tempo_ms =
                (uint32_t)(absolute_time_diff_us(reflexo_inicio, get_absolute_time()) / 1000);

            buzzer_parar();
            reflexo_resultado_tempo(tempo_ms);
        }
        break;

    case REFLEXO_RESULTADO:
        if (btn_a_apertou()) {
            reflexo_nova_rodada();
        }
        break;
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
                simon_iniciar();
            } else if (app.cursor_menu == 1) {
                reflexo_iniciar();
            } else {
                buzzer_tom(880, 100);
                // TODO: implementar historico nos proximos dias
            }
        }
        break;

    case TELA_SIMON:
        tick_simon();
        break;

    case TELA_REFLEXO:
        tick_reflexo();
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

    rng_init();

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