#include "pico/stdlib.h"
#include "menteviva.h"

void npInit(uint pin);
void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b);
void npClear(void);
void npWrite(void);

void matriz_init(void) {
    npInit(LED_MATRIX_PIN);
    matriz_limpar();
}

void matriz_limpar(void) {
    npClear();
    npWrite();
}