#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "hardware/i2c.h"
#include "inc/ssd1306.h"
#include "inc/font.h"
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C

#define PIN_BUTTON_A 5
#define PIN_BUTTON_B 6

int main()
{
  i2c_init(I2C_PORT, 400 * 1000);
  gpio_init(PIN_BUTTON_A);
  gpio_init(PIN_BUTTON_B);
  gpio_set_dir(PIN_BUTTON_A, GPIO_IN);
  gpio_set_dir(PIN_BUTTON_B, GPIO_IN);
  gpio_pull_up(PIN_BUTTON_A);
  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
  gpio_pull_up(I2C_SDA);
  gpio_pull_up(I2C_SCL);
  ssd1306_t ssd;
  ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT);
  ssd1306_config(&ssd);
  ssd1306_send_data(&ssd);

  // Limpa o display. O display inicia com todos os pixels apagados.
  ssd1306_fill(&ssd, true);
  ssd1306_send_data(&ssd);

  bool cor = true;
  int num = 0;
  while (true)
  {
    cor = !cor;
    char text[10];
    sprintf(text, "%d", num);
    // Atualiza o conteúdo do display com animações
    ssd1306_fill(&ssd, !cor); // Limpa o display
    ssd1306_rect(&ssd, 3, 3, 122, 58, cor, !cor); // Desenha um retângulo
    ssd1306_draw_string(&ssd, text, 8, 10); // Desenha uma string
    ssd1306_send_data(&ssd); // Atualiza o display

    if(gpio_get(PIN_BUTTON_A) == 0)
    {
      reset_usb_boot(0,0);
    }
    num++;
    sleep_ms(1000);
  }
}