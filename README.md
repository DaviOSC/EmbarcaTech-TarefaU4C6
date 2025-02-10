# Projeto: Controle de LEDs e Display com RP2040

Este projeto implementa um sistema de controle de LEDs e display utilizando o microcontrolador RP2040, uma matriz de LEDs WS2812, um LED RGB, botões de acionamento e um display SSD1306. O sistema permite a interação com os LEDs e o display através de botões e entrada de caracteres via PC.

## Requisitos

Este projeto envolve a modificação da biblioteca font.h para adicionar caracteres minúsculos, entrada de caracteres via PC para exibição no display SSD1306, interação com os botões A e B para alternar o estado dos LEDs RGB Verde e Azul, respectivamente, uso de interrupções para funcionalidades relacionadas aos botões, implementação de debouncing via software, controle de LEDs comuns e LEDs WS2812, utilização do display 128x64 com fontes maiúsculas e minúsculas, envio de informações pela UART, e organização do código de forma estruturada e comentada.

## Compilação e Execução

1. Clone o repositório do projeto.
2. Configure o ambiente de desenvolvimento do Raspberry Pi Pico.
3. Compile o código usando o CMake.
4. Carregue o binário no Raspberry Pi Pico.

## Estrutura do Código

### Função **desenho_pio**
A função **desenho_pio** é responsável por acionar a matriz de LEDs WS2812. Ela converte os valores RGB em um único valor de 32 bits e envia os dados para a matriz de LEDs.

### Função **gpio_irq_handler**
A função **gpio_irq_handler** é chamada quando um dos botões é pressionado. Ela alterna o estado dos LEDs RGB e exibe uma mensagem no display SSD1306 e no Serial Monitor.

### Função **main**
A função **main** inicializa a comunicação serial, os pinos dos LEDs e botões, e configura o display SSD1306. Em seguida, entra em um loop infinito onde lê caracteres do Serial Monitor e atualiza a matriz de LEDs e o display conforme necessário.

### Debounce do Botão
Para evitar múltiplas leituras do botão devido ao efeito bouncing, foi implementada uma rotina de debounce. Após detectar um clique no botão, o programa espera 300 ms antes de processar a próxima interrupção.

### Link para o vídeo de explicação
https://youtu.be/D_UUy5RDdjI