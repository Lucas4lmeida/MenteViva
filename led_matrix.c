#include <string.h>
#include "pico/stdlib.h"
#include "menteviva.h"
#include "ws2812b.h"

static uint32_t buf[NUM_LEDS];

void matriz_init(void) {
    ws2812b_init(LED_MATRIX_PIN);
    matriz_limpar();
}

void matriz_limpar(void) {
    memset(buf, 0, sizeof(buf));
    ws2812b_set_all(buf, NUM_LEDS);
}
