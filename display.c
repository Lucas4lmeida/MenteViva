#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "menteviva.h"
#include "ssd1306.h"

static ssd1306_t oled;

void display_init(void) {
    i2c_init(OLED_I2C, 400000);
    gpio_set_function(OLED_SDA, GPIO_FUNC_I2C);
    gpio_set_function(OLED_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(OLED_SDA);
    gpio_pull_up(OLED_SCL);
    ssd1306_init(&oled, OLED_W, OLED_H, OLED_ADDR, OLED_I2C);
    ssd1306_clear(&oled);
    ssd1306_show(&oled);
}

void display_boot(void) {
    ssd1306_clear(&oled);
    ssd1306_draw_string(&oled, 12, 8, 2, "MenteViva");
    ssd1306_draw_string(&oled, 4, 32, 1, "Estimulador Cognitivo");
    ssd1306_draw_string(&oled, 16, 48, 1, "Iniciando...");
    ssd1306_show(&oled);
}

void display_menu(uint8_t cursor) {
    ssd1306_clear(&oled);
    ssd1306_draw_string(&oled, 12, 0, 1, "=== MenteViva ===");

    for (int x = 0; x < OLED_W; x++)
        ssd1306_draw_pixel(&oled, x, 10);

    const char *itens[] = {"Memoria", "Reflexo", "Historico"};
    char buf[24];
    for (int i = 0; i < 3; i++) {
        snprintf(buf, sizeof(buf), "%c %s", i == cursor ? '>' : ' ', itens[i]);
        ssd1306_draw_string(&oled, 8, 16 + i * 14, 1, buf);
    }

    ssd1306_draw_string(&oled, 0, 56, 1, "[A]Jogar  [Joy]Mover");
    ssd1306_show(&oled);
}
