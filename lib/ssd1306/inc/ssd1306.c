#include "./ssd1306.h"
#include <stdlib.h>
#include <string.h>

// Corrige init do OLED na BitDogLab; ver docs/problemas/oled-bitdoglab.md
static void ssd1306_write_cmds(ssd1306_t *ssd, const uint8_t *cmds, size_t count) {
    uint8_t buf[17]; // 1 byte de controle + até 16 comandos por vez

    while (count) {
        size_t n = count > 16 ? 16 : count;
        buf[0] = 0x00; // stream de comandos
        memcpy(&buf[1], cmds, n);

        i2c_write_blocking(
            ssd->i2c_port,
            ssd->address,
            buf,
            n + 1,
            false
        );

        cmds += n;
        count -= n;
    }
}

void ssd1306_init(ssd1306_t *ssd, uint8_t width, uint8_t height,
                  bool external_vcc, uint8_t address, i2c_inst_t *i2c) {
    ssd->width = width;
    ssd->height = height;
    ssd->pages = height / 8U;
    ssd->address = address;
    ssd->i2c_port = i2c;
    ssd->external_vcc = external_vcc;

    ssd->bufsize = ssd->pages * ssd->width + 1;
    ssd->ram_buffer = calloc(ssd->bufsize, sizeof(uint8_t));
    ssd->ram_buffer[0] = 0x40; // stream de dados
}

void ssd1306_config(ssd1306_t *ssd) {
    const uint8_t init_cmds[] = {
        SET_DISP | 0x00,
        SET_MEM_ADDR, 0x01,                 // vertical addressing
        SET_DISP_START_LINE | 0x00,
        SET_SEG_REMAP | 0x01,
        SET_MUX_RATIO, (uint8_t)(ssd->height - 1),
        SET_COM_OUT_DIR | 0x08,
        SET_DISP_OFFSET, 0x00,
        SET_COM_PIN_CFG, 0x12,
        SET_DISP_CLK_DIV, 0x80,
        SET_PRECHARGE, 0xF1,
        SET_VCOM_DESEL, 0x30,
        SET_CONTRAST, 0xFF,
        SET_ENTIRE_ON,
        SET_NORM_INV,
        SET_CHARGE_PUMP, 0x14,
        SET_DISP | 0x01
    };

    ssd1306_write_cmds(ssd, init_cmds, sizeof(init_cmds));
}

void ssd1306_command(ssd1306_t *ssd, uint8_t command) {
    ssd1306_write_cmds(ssd, &command, 1);
}

void ssd1306_send_data(ssd1306_t *ssd) {
    const uint8_t addr_cmds[] = {
        SET_COL_ADDR, 0, (uint8_t)(ssd->width - 1),
        SET_PAGE_ADDR, 0, (uint8_t)(ssd->pages - 1)
    };

    ssd1306_write_cmds(ssd, addr_cmds, sizeof(addr_cmds));

    i2c_write_blocking(
        ssd->i2c_port,
        ssd->address,
        ssd->ram_buffer,
        ssd->bufsize,
        false
    );
}

void ssd1306_pixel(ssd1306_t *ssd, uint8_t x, uint8_t y, bool value) {
    if (x >= ssd->width || y >= ssd->height) return;

    uint16_t index = (y >> 3) + (x << 3) + 1;
    uint8_t pixel = (y & 0b111);

    if (value)
        ssd->ram_buffer[index] |= (1 << pixel);
    else
        ssd->ram_buffer[index] &= ~(1 << pixel);
}

void ssd1306_fill(ssd1306_t *ssd, bool value) {
    uint8_t byte = value ? 0xFF : 0x00;
    for (uint16_t i = 1; i < ssd->bufsize; i++) {
        ssd->ram_buffer[i] = byte;
    }
}

void ssd1306_rect(ssd1306_t *ssd, uint8_t top, uint8_t left,
                  uint8_t width, uint8_t height, bool value, bool fill) {
    for (uint8_t x = left; x < left + width; x++) {
        ssd1306_pixel(ssd, x, top, value);
        ssd1306_pixel(ssd, x, top + height - 1, value);
    }

    for (uint8_t y = top; y < top + height; y++) {
        ssd1306_pixel(ssd, left, y, value);
        ssd1306_pixel(ssd, left + width - 1, y, value);
    }

    if (fill) {
        for (uint8_t x = left + 1; x < left + width - 1; x++) {
            for (uint8_t y = top + 1; y < top + height - 1; y++) {
                ssd1306_pixel(ssd, x, y, value);
            }
        }
    }
}

void ssd1306_line(ssd1306_t *ssd, uint8_t x0, uint8_t y0,
                  uint8_t x1, uint8_t y1, bool value) {
    int dx = abs((int)x1 - (int)x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs((int)y1 - (int)y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    while (true) {
        ssd1306_pixel(ssd, x0, y0, value);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void ssd1306_hline(ssd1306_t *ssd, uint8_t x0, uint8_t x1, uint8_t y, bool value) {
    for (uint8_t x = x0; x <= x1; x++) {
        ssd1306_pixel(ssd, x, y, value);
    }
}

void ssd1306_vline(ssd1306_t *ssd, uint8_t x, uint8_t y0, uint8_t y1, bool value) {
    for (uint8_t y = y0; y <= y1; y++) {
        ssd1306_pixel(ssd, x, y, value);
    }
}