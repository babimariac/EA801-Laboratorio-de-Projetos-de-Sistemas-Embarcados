// Inclusão de bibliotecas padrão do C
#include <stdio.h>     // Para funções de entrada/saída como printf
#include <string.h>    // Para manipulação de strings como memset
#include <math.h>      // Para funções matemáticas

// Inclusão de bibliotecas específicas do SDK da Raspberry Pi Pico
#include "pico/stdlib.h"      // Biblioteca padrão do Pico
#include "hardware/adc.h"     // Para leitura do sensor de temperatura interno via ADC
#include "hardware/i2c.h"     // Para comunicação I2C com o display OLED
#include "hardware/pwm.h"     // Para controle PWM dos LEDs RGB
#include "inc/ssd1306.h"      // Biblioteca para controle do display OLED SSD1306

// Definição de constantes para os pinos utilizados
#define I2C_SDA 14            // Pino de dados I2C (Serial Data) para o display
#define I2C_SCL 15            // Pino de clock I2C (Serial Clock) para o display
#define LED_R 13              // Pino do LED vermelho do conjunto RGB
#define LED_G 11              // Pino do LED verde do conjunto RGB
#define LED_B 12              // Pino do LED azul do conjunto RGB
#define ADC_TEMPERATURE_CHANNEL 4  // Canal 4 do ADC conectado ao sensor de temperatura interno
#define ALPHA 0.1f            // Constante para o filtro EMA (Exponential Moving Average)

/*
 * Inicializa um pino para operação PWM
 * Configura o pino para função PWM, define o período e ativa o PWM
 * Retorna o número do slice PWM associado ao pino
 */
uint pwm_init_pin(uint gpio) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);  // Configura o pino para função PWM
    uint slice = pwm_gpio_to_slice_num(gpio); // Obtém o número do slice PWM para o pino
    pwm_set_wrap(slice, 65535);              // Define o valor máximo do contador (16 bits)
    pwm_set_enabled(slice, true);            // Ativa o PWM para o slice
    return slice;
}

/**
 * Define os níveis PWM para o LED RGB
 * Converte valores de 8 bits (0-255) para 16 bits (0-65535) multiplicando por 257
 */
void set_rgb_pwm(uint8_t r, uint8_t g, uint8_t b) {
    pwm_set_gpio_level(LED_R, r * 257);  // Define o nível PWM para o LED vermelho
    pwm_set_gpio_level(LED_G, g * 257);  // Define o nível PWM para o LED verde
    pwm_set_gpio_level(LED_B, b * 257);  // Define o nível PWM para o LED azul
}

/**
 * Converte temperatura de Celsius para Fahrenheit
 */
float celsius_to_fahrenheit(float temperature) {
    return temperature * 9.0f / 5.0f + 32.0f;
}

/**
 * Converte temperatura de Celsius para Kelvin
 */
float celsius_to_kelvin(float temperature) {
    return temperature + 273.15f;
}

/**
 * Converte o valor lido do ADC para temperatura em Celsius
 * Usa a fórmula de conversão específica para o sensor interno do RP2040
 */
float adc_to_temperature(uint16_t adc_value) {
    const float conversion_factor = 3.3f / (1 << 12);  // Fator de conversão para 12 bits (0-4095)
    float voltage = adc_value * conversion_factor;     // Converte leitura ADC para tensão
    return 27.0f - (voltage - 0.706f) / 0.001721f;     // Fórmula de conversão para o sensor interno
}

/**
 * Aplica um filtro de média móvel exponencial (EMA) para suavizar as leituras
 * ALPHA determina o peso da leitura atual vs. leituras anteriores
 */
float apply_ema(float previous, float current) {
    return ALPHA * current + (1.0f - ALPHA) * previous;
}

/**
 * Define a cor do LED RGB com base na temperatura
 * Também define um código de status:
 * 0 = Frio (azul)
 * 1 = Normal (verde)
 * 2 = Quente (vermelho)
 */
void temperature_to_rgb(float temp_c, uint8_t *r, uint8_t *g, uint8_t *b, int *status) {
    if (temp_c <= 25.0f) {
        *r = 0;
        *g = 0;
        *b = 255;      // Azul para temperatura <= 25°C
        *status = 0;   // Status: frio
    } else if (temp_c <= 28.0f) {
        *r = 0;
        *g = 255;
        *b = 0;        // Verde para temperatura entre 25°C e 28°C
        *status = 1;   // Status: normal
    } else {
        *r = 255;
        *g = 0;
        *b = 0;        // Vermelho para temperatura > 28°C
        *status = 2;   // Status: quente
    }
}

int main() {
    // Inicialização do sistema
    stdio_init_all();  // Inicializa entrada/saída padrão (UART, USB)

    // Inicialização do ADC para leitura do sensor de temperatura
    adc_init();
    /*Configura os pinos GPIO 26-29 como entradas analógicas
    Isso ajuda a reduzir o ruído no multiplexador (MUX) do ADC
    mesmo que esses pinos não sejam usados diretamente neste código
    Ao inicializar tais GPIOs, selecionamos a entrada analógica que reduz o ruído no MUX do ADC: */
    adc_gpio_init(26); // Configura GPIO26 (ADC0) como entrada analógica
    adc_gpio_init(27); // Configura GPIO27 (ADC1) como entrada analógica
    adc_gpio_init(28); // Configura GPIO28 (ADC2) como entrada analógica
    adc_gpio_init(29); // Configura GPIO29 (ADC3) como entrada analógica

    adc_set_temp_sensor_enabled(true);  // Ativa o sensor de temperatura interno
    adc_select_input(ADC_TEMPERATURE_CHANNEL);  // Seleciona o canal do sensor de temperatura

    // Inicialização do I2C para comunicação com o display OLED
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);  // Inicializa I2C1 com a velocidade de clock definida
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);  // Configura pino SDA para função I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);  // Configura pino SCL para função I2C
    gpio_pull_up(I2C_SDA);  // Ativa resistor de pull-up interno para SDA
    gpio_pull_up(I2C_SCL);  // Ativa resistor de pull-up interno para SCL
    ssd1306_init();  // Inicializa o display OLED

    // Configuração da área de renderização para o display OLED
    struct render_area frame_area = {
        .start_column = 0,
        .end_column = ssd1306_width - 1,
        .start_page = 0,
        .end_page = ssd1306_n_pages - 1
    };
    calculate_render_area_buffer_length(&frame_area);  // Calcula o tamanho do buffer necessário
    uint8_t ssd[ssd1306_buffer_length];  // Buffer para armazenar dados do display

    // Inicialização dos pinos PWM para o LED RGB
    pwm_init_pin(LED_R);
    pwm_init_pin(LED_G);
    pwm_init_pin(LED_B);

    // Leitura inicial da temperatura e aplicação do filtro
    float filtered_temp = adc_to_temperature(adc_read());

    // Variáveis para controle do piscar dos LEDs
    bool green_led_on = true;
    bool red_led_on = true;
    absolute_time_t last_led_toggle = get_absolute_time();  // Tempo da última alternância do LED
    absolute_time_t last_sensor_read = get_absolute_time(); // Tempo da última leitura do sensor

    // Intervalos de tempo para as operações (em milissegundos)
    const int led_green_interval_ms = 1000;  // LED verde pisca a cada 1 segundo
    const int led_red_interval_ms = 250;     // LED vermelho pisca a cada 0,25 segundos (mais rápido)
    const int sensor_interval_ms = 1000;     // Leitura do sensor a cada 1 segundo

    // Variáveis para armazenar os valores RGB e status
    uint8_t r = 0, g = 0, b = 0;
    int status = 0;

    // Loop principal
    while (true) {
        // Atualiza temperatura a cada 1s
        if (absolute_time_diff_us(last_sensor_read, get_absolute_time()) >= sensor_interval_ms * 1000) {
            // Lê o valor do ADC e converte para temperatura
            float raw_temp = adc_to_temperature(adc_read());
            // Aplica o filtro EMA para suavizar as leituras
            filtered_temp = apply_ema(filtered_temp, raw_temp);

            // Converte a temperatura para diferentes escalas
            float temp_c = filtered_temp;
            float temp_f = celsius_to_fahrenheit(temp_c);
            float temp_k = celsius_to_kelvin(temp_c);

            // Define a cor do LED com base na temperatura
            temperature_to_rgb(temp_c, &r, &g, &b, &status);

            // Prepara as strings para exibição no OLED
            char str_temp_c[20];
            char str_temp_f[20];
            char str_temp_k[20];
            snprintf(str_temp_c, sizeof(str_temp_c), "Temp: %.2f C", temp_c);
            snprintf(str_temp_f, sizeof(str_temp_f), "Temp: %.2f F", temp_f);
            snprintf(str_temp_k, sizeof(str_temp_k), "Temp: %.2f K", temp_k);

            // Limpa o buffer do display e desenha as strings
            memset(ssd, 0, ssd1306_buffer_length);  // Limpa o buffer
            ssd1306_draw_string(ssd, 0, 0, "SENSOR DE TEMP.");  // Título
            ssd1306_draw_string(ssd, 0, 16, str_temp_c);  // Temperatura em Celsius
            ssd1306_draw_string(ssd, 0, 32, str_temp_f);  // Temperatura em Fahrenheit
            ssd1306_draw_string(ssd, 0, 48, str_temp_k);  // Temperatura em Kelvin
            render_on_display(ssd, &frame_area);  // Envia o buffer para o display

            // Imprime informações no console (via UART/USB)
            printf("%s | %s | %s | RGB(%d,%d,%d) | STATUS: %d\n", 
                   str_temp_c, str_temp_f, str_temp_k, r, g, b, status);

            // Atualiza o tempo da última leitura
            last_sensor_read = get_absolute_time();
        }

        // Efeitos visuais — piscar RGB com base no status
        int blink_interval = (status == 2) ? led_red_interval_ms : led_green_interval_ms;
        
        if (status == 0) {
            set_rgb_pwm(r, g, b); // Azul fixo para temperatura baixa (não pisca)
        } else if (absolute_time_diff_us(last_led_toggle, get_absolute_time()) >= blink_interval * 1000) {
            // Seleciona qual LED deve piscar com base no status
            bool *led_state = (status == 1) ? &green_led_on : &red_led_on;
            *led_state = !(*led_state);  // Inverte o estado do LED
            
            if (*led_state) {
                set_rgb_pwm(r, g, b);  // Liga o LED com a cor correspondente
            } else {
                set_rgb_pwm(0, 0, 0);  // Desliga o LED
            }
            
            last_led_toggle = get_absolute_time();  // Atualiza o tempo da última alternância
        }

        sleep_ms(10); // Pequeno delay para evitar uso excessivo de CPU
    }

    return 0;
}