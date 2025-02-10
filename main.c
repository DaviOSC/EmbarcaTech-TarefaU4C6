#include <stdlib.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "hardware/pio.h"
#include "hardware/i2c.h"
#include "inc/ssd1306.h"
#include "inc/font.h"
#include "hardware/clocks.h"
#include "pio_matrix.pio.h"

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C

#define NUM_PIXELS 25
#define BAUD_RATE 115200 // Define a taxa de transmissão

#define PIN_BUTTON_A 5
#define PIN_BUTTON_B 6
#define OUT_PIN 7
#define LED_PIN_GREEN 11
#define LED_PIN_BLUE 12
#define LED_PIN_RED 13

#define DEBOUNCE_TIME_MS 300 // Tempo de debounce em milissegundos

absolute_time_t last_interrupt_time = {0};
ssd1306_t ssd;

bool led_verde_ligado = false;
bool led_azul_ligado = false;

// Matriz de números para exibição
double matrizNumeros[11][25] ={
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
     0.0, 0.1, 0.1, 0.1, 0.0},
     //vazio
     {
      0.0, 0.0, 0.0, 0.0, 0.0,
      0.0, 0.0, 0.0, 0.0, 0.0,
      0.0, 0.0, 0.0, 0.0, 0.0,
      0.0, 0.0, 0.0, 0.0, 0.0,
      0.0, 0.0, 0.0, 0.0, 0.0}
};

// Função para converter valores RGB em um único valor de 32 bits
uint32_t matrix_rgb(double r, double g, double b)
{
  unsigned char R, G, B;
  R = r * 255;
  G = g * 255;
  B = b * 255;
  return (G << 24) | (R << 16) | (B << 8);
}

// Rotina para acionar a matriz de LEDs - ws2812b
void desenho_pio(double *desenho, uint32_t valor_led, PIO pio, uint sm, double r, double g, double b)
{
    for (int16_t i = 0; i < NUM_PIXELS; i++) {
        if (i % 2 == 0)
        {
          valor_led = matrix_rgb(r, g, desenho[24 - i]);
          pio_sm_put_blocking(pio, sm, valor_led);
        }
        else
        {
          valor_led = matrix_rgb(r, desenho[24 - i], b);
          pio_sm_put_blocking(pio, sm, valor_led);
        }
    }
}

// Função de tratamento de interrupção para os botões
static void gpio_irq_handler(uint gpio, uint32_t events)
{
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

  // Verifica qual botão foi pressionado e alterna o estado do LED correspondente
  if (gpio == PIN_BUTTON_A)
  {
    if (led_verde_ligado)
    {
      printf("O led verde foi desligado.\n");
      ssd1306_draw_string(&ssd, "LED Verde OFF", 0, 0);
      led_verde_ligado = false;
    }
    else
    {
      printf("O led verde foi ligado.\n");
      ssd1306_draw_string(&ssd, "LED Verde ON ", 0, 0);
      led_verde_ligado = true;
    }
  }
  else if (gpio == PIN_BUTTON_B)
  {
    if (led_azul_ligado)
    {
      printf("O led azul foi desligado.\n");
      ssd1306_draw_string(&ssd, "LED Azul  OFF", 0, 10);
      led_azul_ligado = false;
    }
    else
    {
      printf("O led azul foi ligado.\n");
      ssd1306_draw_string(&ssd, "LED Azul  ON ", 0, 10);
      led_azul_ligado = true;
    }
  }

  // Atualiza o estado dos LEDs
  gpio_put(LED_PIN_GREEN, led_verde_ligado);
  gpio_put(LED_PIN_BLUE, led_azul_ligado);
  ssd1306_send_data(&ssd);
}

int main()
{
  // Inicialização do PIO
  PIO pio = pio0; 
  uint32_t valor_led;  

  stdio_init_all();

  // Configurações da PIO
  uint offset = pio_add_program(pio, &pio_matrix_program);
  uint sm = pio_claim_unused_sm(pio, true);
  pio_matrix_program_init(pio, sm, offset, OUT_PIN);
  
  // Inicialização dos pinos dos botões e LEDs
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
  
  // Configuração das interrupções dos botões
  gpio_set_irq_enabled_with_callback(PIN_BUTTON_A, GPIO_IRQ_EDGE_FALL, 1, &gpio_irq_handler);
  gpio_set_irq_enabled_with_callback(PIN_BUTTON_B, GPIO_IRQ_EDGE_FALL, 1, &gpio_irq_handler);
  
  // Inicialização do I2C
  i2c_init(I2C_PORT, 400 * 1000);

  // Configuração do I2C para o display SSD1306
  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
  gpio_pull_up(I2C_SDA);
  gpio_pull_up(I2C_SCL);
  
  // Configuração do display SSD1306 
  ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT);
  ssd1306_config(&ssd);
  ssd1306_send_data(&ssd);

  // Limpa o display. O display inicia com todos os pixels apagados.
  ssd1306_fill(&ssd, false);
  ssd1306_send_data(&ssd);

  // Limpa a matriz de LEDs
  desenho_pio(matrizNumeros[10], valor_led, pio, sm, 0, 0, 0);

  while (true)
  {
    char str[2];
    // Lê a string digitada pelo usuário
    fgets(str, sizeof(str), stdin);
    // Lê o primeiro caractere da string
    char c = str[0];

    // Verifica se o caractere é um número e atualiza a matriz de LEDs
    if(c >= '0' && c <= '9')
    {
      desenho_pio(matrizNumeros[c - '0'], valor_led, pio, sm, 0, 0, 0);
    }
    else
    {
      desenho_pio(matrizNumeros[10], valor_led, pio, sm, 0, 0, 0);
    }

    // Desenha o caractere no display
    ssd1306_draw_string(&ssd, &c, 60, 25);
    ssd1306_send_data(&ssd); // Atualiza o display
    // Aguarda 500ms
    sleep_ms(500);
  }
}