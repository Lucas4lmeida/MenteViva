#include "pico/stdlib.h"
#include "menteviva.h"

extern void npInit(uint pin);
extern void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b);
extern void npClear(void);
extern void npWrite(void);

static uint8_t xy(uint8_t x, uint8_t y) {
    return (y & 1) ? (y * 5 + (4 - x)) : (y * 5 + x);
}

// Ajustado para a orientação física observada na placa
static const uint8_t qx[4][4] = {
    {3, 4, 3, 4}, // q0
    {0, 1, 0, 1}, // q1
    {0, 1, 0, 1}, // q2
    {3, 4, 3, 4}  // q3
};

static const uint8_t qy[4][4] = {
    {3, 3, 4, 4}, // q0
    {3, 3, 4, 4}, // q1
    {0, 0, 1, 1}, // q2
    {0, 0, 1, 1}  // q3
};

static const uint8_t cores[4][3] = {
    {0, 40, 0},   // q0 verde
    {0, 0, 40},   // q1 azul
    {40, 0, 0},   // q2 vermelho
    {80, 35, 0}   // q3 amarelo/laranja
};

void matriz_init(void) {
    npInit(LED_MATRIX_PIN);
    matriz_limpar();
}

void matriz_limpar(void) {
    npClear();
    npWrite();
}

void matriz_quadrante(uint8_t q, uint8_t r, uint8_t g, uint8_t b) {
    if (q >= 4) return;

    npClear();

    for (int i = 0; i < 4; i++) {
        uint8_t idx = xy(qx[q][i], qy[q][i]);
        npSetLED(idx, r, g, b);
    }

    npWrite();
}

void matriz_quadrante_padrao(uint8_t q) {
    if (q >= 4) return;
    matriz_quadrante(q, cores[q][0], cores[q][1], cores[q][2]);
}

void matriz_todos_fracos(void) {
    npClear();

    for (uint8_t q = 0; q < 4; q++) {
        for (int i = 0; i < 4; i++) {
            uint8_t idx = xy(qx[q][i], qy[q][i]);
            npSetLED(idx, cores[q][0] / 5, cores[q][1] / 5, cores[q][2] / 5);
        }
    }

    npWrite();
}