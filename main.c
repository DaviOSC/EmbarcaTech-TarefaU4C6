#include <stdlib.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "hardware/pio.h"
#include "hardware/i2c.h"
#include "inc/ssd1306.h"
#include "inc/font.h"
#include "hardware/clocks.h"

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C

#define NUM_PIXELS 25
#define UART_ID uart0    // Seleciona a UART0
#define BAUD_RATE 115200 // Define a taxa de transmissão

#define LED_PIN_GREEN 11
#define LED_PIN_RED 13
#define LED_PIN_BLUE 12
#define PIN_BUTTON_A 5
#define PIN_BUTTON_B 6
#define OUT_PIN 7


#include "pio_matrix.pio.h"


#define DEBOUNCE_TIME_MS 300 // Tempo de debounce em milissegundos
absolute_time_t last_interrupt_time = {0};
ssd1306_t ssd;

bool led_verde_ligado = false;
bool led_azul_ligado = false;

double matrizNumeros[10][25] ={
    // 0
    {0.0, 0.1, 0.1, 0.1, 0.0,
     0.0, 0.1, 0.0, 0.1, 0.0,
     0.0, 0.1, 0.0, 0.1, 0.0,
     0.0, 0.1, 0.0, 0.1, 0.0,
     0.0, 0.1, 0.1, 0.1, 0.0},
    // 1
    {0.0, 0.0, 0.1, 0.0, 0.0,
     0.0, 0.0, 0.1, 0.1, 0.0,
     0.0, 0.0, 0.1, 0.0, 0.0,
     0.0, 0.0, 0.1, 0.0, 0.0,
     0.0, 0.1, 0.1, 0.1, 0.0},
    // 2
    {0.0, 0.1, 0.1, 0.1, 0.0,
     0.0, 0.1, 0.0, 0.0, 0.0,
     0.0, 0.1, 0.1, 0.1, 0.0,
     0.0, 0.0, 0.0, 0.1, 0.0,
     0.0, 0.1, 0.1, 0.1, 0.0},
    // 3
    {0.0, 0.1, 0.1, 0.1, 0.0,
     0.0, 0.1, 0.0, 0.0, 0.0,
     0.0, 0.1, 0.1, 0.1, 0.0,
     0.0, 0.1, 0.0, 0.0, 0.0,
     0.0, 0.1, 0.1, 0.1, 0.0},
    // 4
    {0.0, 0.1, 0.0, 0.1, 0.0,
     0.0, 0.1, 0.0, 0.1, 0.0,
     0.0, 0.1, 0.1, 0.1, 0.0,
     0.0, 0.1, 0.0, 0.0, 0.0,
     0.0, 0.0, 0.0, 0.1, 0.0},
    // 5
    {0.0, 0.1, 0.1, 0.1, 0.0,
     0.0, 0.0, 0.0, 0.1, 0.0,
     0.0, 0.1, 0.1, 0.1, 0.0,
     0.0, 0.1, 0.0, 0.0, 0.0,
     0.0, 0.1, 0.1, 0.1, 0.0},
    // 6
    {0.0, 0.1, 0.1, 0.1, 0.0,
     0.0, 0.0, 0.0, 0.1, 0.0,
     0.0, 0.1, 0.1, 0.1, 0.0,
     0.0, 0.1, 0.0, 0.1, 0.0,
     0.0, 0.1, 0.1, 0.1, 0.0},
    // 7
    {0.0, 0.1, 0.1, 0.1, 0.0,
     0.0, 0.1, 0.0, 0.0, 0.0,
     0.0, 0.0, 0.0, 0.1, 0.0,
     0.0, 0.1, 0.0, 0.0, 0.0,
     0.0, 0.0, 0.0, 0.1, 0.0},
    // 8
    {0.0, 0.1, 0.1, 0.1, 0.0,
     0.0, 0.1, 0.0, 0.1, 0.0,
     0.0, 0.1, 0.1, 0.1, 0.0,
     0.0, 0.1, 0.0, 0.1, 0.0,
     0.0, 0.1, 0.1, 0.1, 0.0},
    // 9
    {0.0, 0.1, 0.1, 0.1, 0.0,
     0.0, 0.1, 0.0, 0.1, 0.0,
     0.0, 0.1, 0.1, 0.1, 0.0,
     0.0, 0.1, 0.0, 0.0, 0.0,
     0.0, 0.1, 0.1, 0.1, 0.0}
};

uint32_t matrix_rgb(double b, double r, double g)
{
  unsigned char R, G, B;
  R = r * 255;
  G = g * 255;
  B = b * 255;
  return (G << 24) | (R << 16) | (B << 8);
}

//rotina para acionar a matrix de leds - ws2812b
void desenho_pio(double *desenho, uint32_t valor_led, PIO pio, uint sm, double r, double g, double b){

    for (int16_t i = 0; i < NUM_PIXELS; i++) {
        if (i%2==0)
        {
            valor_led = matrix_rgb(desenho[24-i], g, b);
            pio_sm_put_blocking(pio, sm, valor_led);

        }else{
            valor_led = matrix_rgb(r, desenho[24-i], b);
            pio_sm_put_blocking(pio, sm, valor_led);
        }
    }
}
static void gpio_irq_handler(uint gpio, uint32_t events)
{
  ssd1306_fill(&ssd, false); // Limpa o display

  // Obter o tempo atual para o debounce
  absolute_time_t current_time = get_absolute_time();

  if (absolute_time_diff_us(last_interrupt_time, current_time) < DEBOUNCE_TIME_MS * 1000)
  {
    return; // Ignora a interrupção se estiver dentro do tempo de debounce
  }
  else
  {
    last_interrupt_time = current_time;
  }

  if (gpio == PIN_BUTTON_A)
  {
    printf("Interrupção ocorreu no pino %d, no evento %d\n", gpio, events);
    printf("Botão A pressionado\n");

    if (led_verde_ligado)
    {
      printf("O led verde foi desligado.\n");
      ssd1306_draw_string(&ssd, "LED Verde: OFF", 0, 0);
      led_verde_ligado = false;
    }
    else
    {
      printf("O led verde foi ligado.\n");
      ssd1306_draw_string(&ssd, "LED Verde: ON", 0, 0);
      led_verde_ligado = true;
    }
  }
  else if (gpio == PIN_BUTTON_B)
  {
    printf("Interrupção ocorreu no pino %d, no evento %d\n", gpio, events);
    printf("Botão B pressionado\n");

    if (led_azul_ligado)
    {
      printf("O led azul foi desligado.\n");
      ssd1306_draw_string(&ssd, "LED Azul: OFF", 0, 0);
      led_azul_ligado = false;
    }
    else
    {
      printf("O led azul foi ligado.\n");
      ssd1306_draw_string(&ssd, "LED Azul: ON", 0, 0);
      led_azul_ligado = true;
    }
  }

  gpio_put(LED_PIN_GREEN, led_verde_ligado);
  gpio_put(LED_PIN_BLUE, led_azul_ligado);
  ssd1306_send_data(&ssd);
}

int main()
{
  PIO pio = pio0; 
  bool ok;
  uint16_t i;
  uint32_t valor_led;
  double r = 0.0, b = 0.0 , g = 0.0;
  
  stdio_init_all();

  //configurações da PIO
    uint offset = pio_add_program(pio, &pio_matrix_program);
    uint sm = pio_claim_unused_sm(pio, true);
    pio_matrix_program_init(pio, sm, offset, OUT_PIN);
  i2c_init(I2C_PORT, 400 * 1000);
  
  gpio_init(PIN_BUTTON_A);
  gpio_init(PIN_BUTTON_B);
  gpio_init(LED_PIN_GREEN);
  gpio_init(LED_PIN_BLUE);
  gpio_set_dir(PIN_BUTTON_A, GPIO_IN);
  gpio_set_dir(PIN_BUTTON_B, GPIO_IN);
  gpio_set_dir(LED_PIN_GREEN, GPIO_OUT);
  gpio_set_dir(LED_PIN_BLUE, GPIO_OUT);
  gpio_pull_up(PIN_BUTTON_A);
  gpio_pull_up(PIN_BUTTON_B);

  gpio_set_irq_enabled_with_callback(PIN_BUTTON_A, GPIO_IRQ_EDGE_FALL, 1, &gpio_irq_handler);
  gpio_set_irq_enabled_with_callback(PIN_BUTTON_B, GPIO_IRQ_EDGE_FALL, 1, &gpio_irq_handler);


  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
  gpio_pull_up(I2C_SDA);
  gpio_pull_up(I2C_SCL);
  ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT);
  ssd1306_config(&ssd);
  ssd1306_send_data(&ssd);

  // Limpa o display. O display inicia com todos os pixels apagados.
  ssd1306_fill(&ssd, false);
  ssd1306_send_data(&ssd);

  bool cor = true;
  while (true)
  {
    cor = !cor;

    // Atualiza o conteúdo do display com animações
    //ssd1306_fill(&ssd, !cor); // Limpa o display
    ssd1306_rect(&ssd, 0, 0, 122, 58, cor, !cor); // Desenha um retângulo
    char c;
    scanf("%c", &c);
    char str[2] = {c, '\0'};
    if(str[0] >= '0' && str[0] <= '9')
    {
      desenho_pio(matrizNumeros[c - '0'], valor_led, pio, sm, r, g, b);
    }
    ssd1306_draw_string(&ssd, str, 60, 25); // Desenha uma string
    ssd1306_send_data(&ssd); // Atualiza o display

    if(gpio_get(PIN_BUTTON_A) == 0)
    {
      reset_usb_boot(0,0);
    }
    
    sleep_ms(500);
  }
}