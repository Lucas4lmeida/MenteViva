#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "inc/ssd1306.h"

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15

int main() {
  stdio_init_all();

  // I2C Initialisation. Using it at 400Khz.
  i2c_init(I2C_PORT, 400*1000);
  
  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
  gpio_pull_up(I2C_SDA);
  gpio_pull_up(I2C_SCL);

  ssd1306_t ssd;

  ssd1306_init(&ssd, WIDTH, HEIGHT, false, 0x3C, I2C_PORT);
  ssd1306_config(&ssd);

  ssd1306_send_data(&ssd);

  ssd1306_fill(&ssd, false);
  ssd1306_hline(&ssd, 0, 127, 32, true);
  ssd1306_vline(&ssd, 63, 0, 63, true);
  //ssd1306_draw_string(&ssd, 0, 0, "Bem-vindos!");
  //ssd1306_draw_string(&ssd, 0, 10, "Embarcatech");
  ssd1306_send_data(&ssd); // Atualiza o display

    ssd1306_draw_string(&ssd, 0, 0, "Bem-vindos!");
    ssd1306_draw_string(&ssd, 0, 10, "Embarcatech");
    ssd1306_send_data(&ssd); // Atualiza o display

  ssd1306_send_data(&ssd);

  while (true) {
    sleep_ms(1000);
  }
}
