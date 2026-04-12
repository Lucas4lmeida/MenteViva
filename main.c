#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "hardware/regs/addressmap.h"
#include "menteviva.h"

app_t app;

#define SIMON_MAX_NIVEIS       5
#define REFLEXO_RODADAS        5
#define REFLEXO_PENALIDADE_MS  1000
#define HIST_MAX               6
#define DADOS_MAGIC            0x4D563631u
#define DADOS_VERSAO           1
#define FLASH_TARGET_OFFSET    (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)

typedef enum {
    REFLEXO_ESPERANDO = 0,
    REFLEXO_PRONTO,
    REFLEXO_FEEDBACK,
    REFLEXO_FINAL
} reflexo_estado_t;

typedef enum {
    TEND_ND = 0,
    TEND_MEL,
    TEND_EST,
    TEND_QDA
} tendencia_t;

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t reservado0;
    uint8_t  simon_count;
    uint8_t  reflexo_count;
    uint8_t  reservado1[2];
    uint8_t  simon_hist[HIST_MAX];
    uint8_t  reservado2[2];
    uint16_t reflexo_hist[HIST_MAX];
    uint32_t checksum;
} flash_dados_t;

static uint8_t simon_seq[SIMON_MAX_NIVEIS];
static uint8_t simon_nivel = 1;
static uint8_t simon_indice = 0;
static bool simon_joy_livre = false;

static reflexo_estado_t reflexo_estado = REFLEXO_ESPERANDO;
static uint8_t reflexo_q = 0;
static uint8_t reflexo_rodada = 1;
static uint8_t reflexo_antecipacoes = 0;
static uint32_t reflexo_soma_ms = 0;
static uint32_t reflexo_deadline = 0;
static uint32_t reflexo_feedback_ate = 0;
static uint32_t reflexo_inicio_us = 0;

static flash_dados_t dados_flash;
static uint8_t flash_setor[FLASH_SECTOR_SIZE];

static uint32_t ultimo_mov = 0;

static tendencia_t tendencia_simon(void);
static tendencia_t tendencia_reflexo(void);
static const char *tendencia_txt(tendencia_t t);
static int simon_ult(void);
static int reflexo_ult(void);
static void atualizar_historico(void);
static void entrar_historico(void);

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

static uint32_t flash_checksum(const flash_dados_t *d) {
    const uint8_t *p = (const uint8_t *)d;
    uint32_t acc = 5381u;

    for (size_t i = 0; i < offsetof(flash_dados_t, checksum); i++) {
        acc = ((acc << 5) + acc) ^ p[i];
    }

    return acc;
}

static void flash_padrao(void) {
    memset(&dados_flash, 0, sizeof(dados_flash));
    dados_flash.magic = DADOS_MAGIC;
    dados_flash.version = DADOS_VERSAO;
}

static bool flash_valido(const flash_dados_t *d) {
    if (d->magic != DADOS_MAGIC) return false;
    if (d->version != DADOS_VERSAO) return false;
    if (d->simon_count > HIST_MAX) return false;
    if (d->reflexo_count > HIST_MAX) return false;
    if (d->checksum != flash_checksum(d)) return false;
    return true;
}

static void flash_carregar(void) {
    const flash_dados_t *gravado = (const flash_dados_t *)(XIP_BASE + FLASH_TARGET_OFFSET);

    if (flash_valido(gravado)) {
        dados_flash = *gravado;
        printf("[flash] carregado s=%d r=%d\n", dados_flash.simon_count, dados_flash.reflexo_count);
    } else {
        flash_padrao();
        printf("[flash] vazio/invalido\n");
    }
}

static void flash_salvar(void) {
    dados_flash.magic = DADOS_MAGIC;
    dados_flash.version = DADOS_VERSAO;
    dados_flash.checksum = flash_checksum(&dados_flash);

    memset(flash_setor, 0xFF, sizeof(flash_setor));
    memcpy(flash_setor, &dados_flash, sizeof(dados_flash));

    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(FLASH_TARGET_OFFSET, flash_setor, FLASH_SECTOR_SIZE);
    restore_interrupts(ints);

    printf("[flash] salvo\n");
}

static void hist_add_u8(uint8_t *hist, uint8_t *count, uint8_t valor) {
    if (*count < HIST_MAX) {
        hist[*count] = valor;
        (*count)++;
    } else {
        memmove(hist, hist + 1, HIST_MAX - 1);
        hist[HIST_MAX - 1] = valor;
    }
}

static void hist_add_u16(uint16_t *hist, uint8_t *count, uint16_t valor) {
    if (*count < HIST_MAX) {
        hist[*count] = valor;
        (*count)++;
    } else {
        memmove(hist, hist + 1, (HIST_MAX - 1) * sizeof(uint16_t));
        hist[HIST_MAX - 1] = valor;
    }
}

static void registrar_score_simon(uint8_t score) {
    hist_add_u8(dados_flash.simon_hist, &dados_flash.simon_count, score);
    
    // desabilita interrupcoes: flash XIP fica inacessivel durante erase/write
    flash_salvar();

    wifi_atualizar_dashboard(
        simon_ult(),
        tendencia_txt(tendencia_simon()),
        reflexo_ult(),
        tendencia_txt(tendencia_reflexo())
    );

    printf("[score] simon=%d\n", score);
}

static void registrar_score_reflexo(uint16_t media_ms) {
    hist_add_u16(dados_flash.reflexo_hist, &dados_flash.reflexo_count, media_ms);
    flash_salvar();

    wifi_atualizar_dashboard(
        simon_ult(),
        tendencia_txt(tendencia_simon()),
        reflexo_ult(),
        tendencia_txt(tendencia_reflexo())
    );

    printf("[score] reflexo=%d ms\n", media_ms);
}

static tendencia_t tendencia_simon(void) {
    if (dados_flash.simon_count < 6) return TEND_ND;

    uint32_t prev =
        dados_flash.simon_hist[dados_flash.simon_count - 6] +
        dados_flash.simon_hist[dados_flash.simon_count - 5] +
        dados_flash.simon_hist[dados_flash.simon_count - 4];

    uint32_t recent =
        dados_flash.simon_hist[dados_flash.simon_count - 3] +
        dados_flash.simon_hist[dados_flash.simon_count - 2] +
        dados_flash.simon_hist[dados_flash.simon_count - 1];

    if (recent * 100 >= prev * 115) return TEND_MEL;
    if (recent * 100 <= prev * 85)  return TEND_QDA;
    return TEND_EST;
}

static tendencia_t tendencia_reflexo(void) {
    if (dados_flash.reflexo_count < 6) return TEND_ND;

    uint32_t prev =
        dados_flash.reflexo_hist[dados_flash.reflexo_count - 6] +
        dados_flash.reflexo_hist[dados_flash.reflexo_count - 5] +
        dados_flash.reflexo_hist[dados_flash.reflexo_count - 4];

    uint32_t recent =
        dados_flash.reflexo_hist[dados_flash.reflexo_count - 3] +
        dados_flash.reflexo_hist[dados_flash.reflexo_count - 2] +
        dados_flash.reflexo_hist[dados_flash.reflexo_count - 1];

    if (recent * 100 <= prev * 85) return TEND_MEL;
    if (recent * 100 >= prev * 115) return TEND_QDA;
    return TEND_EST;
}

static const char *tendencia_txt(tendencia_t t) {
    switch (t) {
        case TEND_MEL: return "MEL";
        case TEND_EST: return "EST";
        case TEND_QDA: return "QDA";
        default:       return "N/D";
    }
}

static int simon_ult(void) {
    if (dados_flash.simon_count == 0) return -1;
    return dados_flash.simon_hist[dados_flash.simon_count - 1];
}

static int reflexo_ult(void) {
    if (dados_flash.reflexo_count == 0) return -1;
    return dados_flash.reflexo_hist[dados_flash.reflexo_count - 1];
}

static void atualizar_historico(void) {
    display_historico(
        simon_ult(),
        tendencia_txt(tendencia_simon()),
        reflexo_ult(),
        tendencia_txt(tendencia_reflexo())
    );
}

static void entrar_historico(void) {
    app.tela = TELA_HISTORICO;
    matriz_limpar();
    rgb_set(0, 0, 20);
    atualizar_historico();
    printf("[hist] aberto\n");
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

    while (joy_direcao() != DIR_NENHUM || joy_quadrante_diagonal() >= 0) {
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

    registrar_score_simon(score);

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

    registrar_score_simon(SIMON_MAX_NIVEIS);

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

    int8_t q = joy_quadrante_diagonal();

    if (q < 0) {
        simon_joy_livre = true;
        return;
    }

    if (!simon_joy_livre) return;
    simon_joy_livre = false;

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

static void reflexo_agendar_rodada(void) {
    reflexo_q = rand() % 4;
    reflexo_deadline = agora_ms() + 1200 + (rand() % 1800);
    reflexo_estado = REFLEXO_ESPERANDO;

    matriz_limpar();
    rgb_set(10, 0, 10);
    display_reflexo_aguarde(reflexo_rodada);

    printf("[reflexo] rodada=%d alvo=%d\n", reflexo_rodada, reflexo_q);
}

static void reflexo_iniciar(void) {
    app.tela = TELA_REFLEXO;
    reflexo_rodada = 1;
    reflexo_antecipacoes = 0;
    reflexo_soma_ms = 0;
    reflexo_feedback_ate = 0;

    printf("[reflexo] iniciar\n");
    reflexo_agendar_rodada();
}

static void reflexo_finalizar_partida(void) {
    uint32_t media = reflexo_soma_ms / REFLEXO_RODADAS;

    registrar_score_reflexo((uint16_t)media);

    matriz_limpar();
    rgb_set(0, 20, 0);
    display_reflexo_final(media, reflexo_antecipacoes);
    reflexo_estado = REFLEXO_FINAL;

    printf("[reflexo] fim media=%lu antecip=%d\n", media, reflexo_antecipacoes);
}

static void reflexo_avancar_ou_finalizar(void) {
    if (reflexo_rodada >= REFLEXO_RODADAS) {
        reflexo_finalizar_partida();
        return;
    }

    reflexo_rodada++;
    reflexo_agendar_rodada();
}

static void reflexo_feedback_tempo(uint32_t tempo_ms) {
    reflexo_soma_ms += tempo_ms;

    matriz_limpar();
    rgb_set(0, 20, 0);
    display_reflexo_parcial(reflexo_rodada, false, tempo_ms);
    reflexo_estado = REFLEXO_FEEDBACK;
    reflexo_feedback_ate = agora_ms() + 900;

    printf("[reflexo] tempo=%lu ms\n", tempo_ms);
}

static void reflexo_feedback_antecipou(void) {
    reflexo_soma_ms += REFLEXO_PENALIDADE_MS;
    reflexo_antecipacoes++;

    matriz_limpar();
    rgb_set(20, 0, 0);
    display_reflexo_parcial(reflexo_rodada, true, REFLEXO_PENALIDADE_MS);
    reflexo_estado = REFLEXO_FEEDBACK;
    reflexo_feedback_ate = agora_ms() + 900;

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
            buzzer_tom(220, 200);
            reflexo_feedback_antecipou();
            return;
        }

        if (agora_ms() >= reflexo_deadline) {
            reflexo_estado = REFLEXO_PRONTO;

            matriz_quadrante_padrao(reflexo_q);
            reflexo_inicio_us = time_us_32();

            display_reflexo_pronto(reflexo_rodada);
            buzzer_simon_tom(reflexo_q);
            rgb_set(0, 0, 20);

            printf("[reflexo] alvo liberado q=%d\n", reflexo_q);
        }
        break;

    case REFLEXO_PRONTO:
        if (btn_a_apertou()) {
            uint32_t tempo_ms = (btn_a_tempo_us() - reflexo_inicio_us) / 1000;

            buzzer_parar();
            reflexo_feedback_tempo(tempo_ms);
        }
        break;

    case REFLEXO_FEEDBACK:
        if (agora_ms() >= reflexo_feedback_ate) {
            reflexo_avancar_ou_finalizar();
        }
        break;

    case REFLEXO_FINAL:
        if (btn_a_apertou()) {
            reflexo_iniciar();
        }
        break;
    }
}

static void tick_historico(void) {
    atualizar_historico();

    if (btn_b_apertou()) {
        printf("[hist] saiu\n");
        voltar_menu();
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
                entrar_historico();
            }
        }
        break;

    case TELA_SIMON:
        tick_simon();
        break;

    case TELA_REFLEXO:
        tick_reflexo();
        break;

    case TELA_HISTORICO:
        tick_historico();
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

    flash_carregar();

    wifi_atualizar_dashboard(
        simon_ult(),
        tendencia_txt(tendencia_simon()),
        reflexo_ult(),
        tendencia_txt(tendencia_reflexo())
    );
    rgb_set(0, 0, 20);
    display_wifi_status(false, NULL);
    sleep_ms(200);

    wifi_init();

    if (wifi_ativo()) {
    rgb_set(0, 20, 0);
    display_wifi_status(true, wifi_ip_str());
    printf("[wifi] status: conectado (%s)\n", wifi_ip_str());
    } else {
    rgb_set(20, 0, 0);
    display_wifi_status(false, NULL);
    printf("[wifi] status: falhou\n");
}

sleep_ms(1500);

    sleep_ms(2000);
    app.tela = TELA_BOOT;

    rgb_set(0, 10, 0);
    printf("[init] pronto!\n");

    buzzer_tom(660, 100);
    buzzer_tom(880, 100);
    buzzer_tom(1100, 200);

    while (true) {
        wifi_poll();
        buzzer_tick();
        maquina_estados();
        sleep_ms(10);
    }
}