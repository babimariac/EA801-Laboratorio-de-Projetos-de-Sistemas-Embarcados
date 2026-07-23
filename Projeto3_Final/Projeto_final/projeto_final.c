// Feito por Bárbara Maria B. F. de Cerqueira César e Glauco César Prado Soares
// Biblioteca usada para o DHT: https://github.com/vmilea/pico_dht
// Inclusão de bibliotecas padrão do C
#include <stdio.h>
#include <string.h>
#include <math.h>

// Inclusão de bibliotecas específicas do SDK da Raspberry Pi Pico
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "inc/ssd1306.h"
#include "dht.h"  // Biblioteca DHT
#include "pico/cyw43_arch.h" // Para Wi-Fi
#include "lwip/tcp.h" // Para servidor TCP/HTTP
#include "pico/multicore.h" //Para rodar Wi-Fi no outro core


// Definição de constantes para os pinos utilizados
#define I2C_SDA 14
#define I2C_SCL 15
#define LED_R 13
#define LED_G 11
#define LED_B 12
#define ADC_TEMPERATURE_CHANNEL 4
#define DHT_PIN 19  // Pino do sensor DHT22
#define ALPHA 0.1f
#define BUTTON_A_PIN 5  // Botão A para alternar tela para frente
#define BUTTON_B_PIN 6  // Botão B para alternar tela para trás
#define GRAPH_WIDTH 128  // Largura do gráfico para histórico
#define PWM_OUTPUT_PIN 20  // Define o pino do PWM

// Configurações Wi-Fi - ALTERE PARA SUA REDE
/*#define WIFI_SSID "gabinete"
#define WIFI_PASS "nitro3859"*/

#define WIFI_SSID "Galaxy A53 5G 9009"
#define WIFI_PASS "lqfd2838"

/*#define WIFI_SSID "AndroidAP7A01"
#define WIFI_PASS "kmme9305"*/

// Definição do modelo do sensor DHT
static const dht_model_t DHT_MODEL = DHT22;

// Variáveis globais para armazenar os dados de temperatura e umidade
// Estas variáveis serão acessadas pelo servidor HTTP
volatile float g_internal_temp_c = 0.0f;
volatile float g_external_temp_c = 0.0f;
volatile float g_humidity = 0.0f;
volatile int g_dht_status = 0; // 0 = erro, 1 = ok

// Variáveis para controle da tela e estado dos botões
int screen_mode = 0;  // Modo inicial da tela
static bool last_a_state = true;  // Estado anterior do botão A
static bool last_b_state = true;  // Estado anterior do botão B

// Buffers para histórico de dados para os gráficos
float temp_history[GRAPH_WIDTH] = {0};
float ext_temp_history[GRAPH_WIDTH] = {0};
float humidity_history[GRAPH_WIDTH] = {0};

// Buffer para a página HTML
static char html_buffer[2048];

// Funções existentes - mantidas sem alteração
uint pwm_init_pin(uint gpio) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(gpio);
    pwm_set_wrap(slice, 65535);
    pwm_set_enabled(slice, true);
    return slice;
}

void set_rgb_pwm(uint8_t r, uint8_t g, uint8_t b) {
    pwm_set_gpio_level(LED_R, r * 257);
    pwm_set_gpio_level(LED_G, g * 257);
    pwm_set_gpio_level(LED_B, b * 257);
}

float celsius_to_fahrenheit(float temperature) {
    return temperature * 9.0f / 5.0f + 32.0f;
}

float celsius_to_kelvin(float temperature) {
    return temperature + 273.15f;
}

float adc_to_temperature(uint16_t adc_value) {
    const float conversion_factor = 3.3f / (1 << 12);
    float voltage = adc_value * conversion_factor;
    return 27.0f - (voltage - 0.706f) / 0.001721f;
}

float apply_ema(float previous, float current) {
    return ALPHA * current + (1.0f - ALPHA) * previous;
}

void temperature_to_rgb(float temp_c, uint8_t *r, uint8_t *g, uint8_t *b, int *status) {
    if (temp_c <= 25.0f) {
        *r = 0;
        *g = 0;
        *b = 255;
        *status = 0;
    } else if (temp_c <= 28.0f) {
        *r = 0;
        *g = 255;
        *b = 0;
        *status = 1;
    } else {
        *r = 255;
        *g = 0;
        *b = 0;
        *status = 2;
    }
}

// === Funções para o servidor HTTP ===
static err_t http_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (!p) {
        tcp_close(tpcb);
        return ERR_OK;
    }

    // Atualiza a página HTML com os dados atuais
    snprintf(html_buffer, sizeof(html_buffer),
         "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
         "<!DOCTYPE html><html><head>"
         "<meta charset='UTF-8'>"
         "<meta http-equiv='refresh' content='2'>"
         "<title>Monitor de Temperatura</title>"
         "<style>"
         "body { font-family: Arial, sans-serif; margin: 0; padding: 20px; background-color: #f5f5f5; }"
         ".container { max-width: 800px; margin: 0 auto; background-color: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }"
         "h1 { color: #333; text-align: center; }"
         ".temp-card { background-color: #e9f5ff; padding: 15px; margin: 10px 0; border-radius: 5px; }"
         ".temp-card.external { background-color: #e9ffe9; }"
         ".temp-value { font-size: 24px; font-weight: bold; margin: 10px 0; }"
         ".temp-unit { font-size: 18px; color: #666; }"
         ".humidity { background-color: #f0e9ff; padding: 15px; margin: 10px 0; border-radius: 5px; }"
         "</style>"
         "</head><body>"
         "<div class='container'>"
         "<h1>Monitor de Temperatura Raspberry Pi Pico W</h1>"
         
         "<div class='temp-card'>"
         "<h2>Temperatura Interna (RP2040)</h2>"
         "<div class='temp-value'>%.1f °C</div>"
         "<div class='temp-unit'>%.1f °F / %.1f K</div>"
         "</div>"
         
         "<div class='temp-card external'>"
         "<h2>Temperatura Externa (DHT22)</h2>",
         g_internal_temp_c, celsius_to_fahrenheit(g_internal_temp_c), celsius_to_kelvin(g_internal_temp_c));
    
    // Adiciona os dados do sensor DHT22 se estiverem disponíveis
    if (g_dht_status) {
        char temp_section[512];
        snprintf(temp_section, sizeof(temp_section),
                 "<div class='temp-value'>%.1f °C</div>"
                 "<div class='temp-unit'>%.1f °F / %.1f K</div>"
                 "</div>"
                 
                 "<div class='humidity'>"
                 "<h2>Umidade</h2>"
                 "<div class='temp-value'>%.1f%%</div>"
                 "</div>",
                 g_external_temp_c, 
                 celsius_to_fahrenheit(g_external_temp_c),
                 celsius_to_kelvin(g_external_temp_c),
                 g_humidity);
        strcat(html_buffer, temp_section);
    } else {
        strcat(html_buffer, 
               "<div class='temp-value'>Sensor não disponível</div>"
               "</div>");
    }
    
    // Finaliza a página HTML
    strcat(html_buffer, 
           "</div>"
           "<script>"
           "setTimeout(function() { location.reload(); }, 2000);"
           "</script>"
           "</body></html>\r\n");

    // Libera o buffer de recepção e envia a resposta HTTP
    pbuf_free(p);
    tcp_write(tpcb, html_buffer, strlen(html_buffer), TCP_WRITE_FLAG_COPY);
    
    return ERR_OK;
}

static err_t connection_callback(void *arg, struct tcp_pcb *newpcb, err_t err) {
    tcp_recv(newpcb, http_callback);
    return ERR_OK;
}

static void start_http_server(void) {
    struct tcp_pcb *pcb = tcp_new();
    tcp_bind(pcb, IP_ADDR_ANY, 80);
    pcb = tcp_listen(pcb);
    tcp_accept(pcb, connection_callback);
}

// Função para rodar o Wi-Fi em paralelo no Core 1
void wifi_task() {
    if (cyw43_arch_init()) {
        printf("Erro ao inicializar o Wi-Fi\n");
        return;
    }
    cyw43_arch_enable_sta_mode();

    while (true) {
        printf("Tentando conectar ao Wi-Fi...\n");
        if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 5000) == 0) {
            printf("Wi-Fi conectado!\n");
            
            uint8_t *ip = (uint8_t*)&(cyw43_state.netif[0].ip_addr.addr);
            printf("Endereço IP: %d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);

            start_http_server(); // *NOVO: Inicia o servidor HTTP assim que conectar

            break;
        }

        printf("Falha na conexão, tentando novamente...\n");
        sleep_ms(2000); // Pequena espera antes de tentar de novo
    }

    while (true) {
        cyw43_arch_poll();  // Mantém a conexão Wi-Fi ativa

        static uint64_t last_print_time = 0;
        uint64_t now = time_us_64();

        // Print do IP a cada 30 segundos
        if (now - last_print_time >= 5 * 1000000) { 
            uint8_t *ip = (uint8_t*)&(cyw43_state.netif[0].ip_addr.addr);
            printf("[Wi-Fi] Endereço IP: %d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);
            last_print_time = now;
        }

        sleep_ms(100);      // Pequena espera para evitar uso excessivo da CPU
    }
}

int main() {
    // Inicialização do sistema
    stdio_init_all();
    sleep_ms(5000);  // Pequeno delay para inicialização
    
    printf("Iniciando sistema de monitoramento de temperatura...\n");

    multicore_launch_core1(wifi_task);

    // Inicialização do ADC para leitura do sensor de temperatura
    adc_init();
    adc_gpio_init(26);
    adc_gpio_init(27);
    adc_gpio_init(28);
    adc_gpio_init(29);
    adc_set_temp_sensor_enabled(true);
    adc_select_input(ADC_TEMPERATURE_CHANNEL);

    // Inicialização do I2C para comunicação com o display OLED
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    ssd1306_init();

    // Configuração da área de renderização para o display OLED
    struct render_area frame_area = {
        .start_column = 0,
        .end_column = ssd1306_width - 1,
        .start_page = 0,
        .end_page = ssd1306_n_pages - 1
    };
    calculate_render_area_buffer_length(&frame_area);
    uint8_t ssd[ssd1306_buffer_length];

    // Inicialização dos pinos PWM para o LED RGB
    pwm_init_pin(LED_R);
    pwm_init_pin(LED_G);
    pwm_init_pin(LED_B);
    
    // PWM na GPIO definida pela constante PWM_OUTPUT_PIN - 100kHz, duty 55%
    gpio_set_function(PWM_OUTPUT_PIN, GPIO_FUNC_PWM);
    uint pwm_slice = pwm_gpio_to_slice_num(PWM_OUTPUT_PIN);
    pwm_set_wrap(pwm_slice, 1249);  // Frequência = 100kHz
    pwm_set_chan_level(pwm_slice, pwm_gpio_to_channel(PWM_OUTPUT_PIN), 688); // Duty cycle = 55%
    pwm_set_enabled(pwm_slice, true);

    // Inicialização dos botões A e B
    gpio_init(BUTTON_A_PIN);
    gpio_init(BUTTON_B_PIN);
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
    gpio_set_dir(BUTTON_B_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_A_PIN);
    gpio_pull_up(BUTTON_B_PIN);

    // Inicialização do sensor DHT22
    dht_t dht;
    dht_init(&dht, DHT_MODEL, pio0, DHT_PIN, true /* pull_up */);

    // Leitura inicial da temperatura e aplicação do filtro
    float filtered_temp = adc_to_temperature(adc_read());
    g_internal_temp_c = filtered_temp;

    // Inicialização do histórico de temperaturas
    for (int i = 0; i < GRAPH_WIDTH; i++) {
        temp_history[i] = filtered_temp;
        ext_temp_history[i] = 0;
        humidity_history[i] = 0;
    }

    // Variáveis para armazenar os dados do DHT22
    float dht_humidity = 0.0f;
    float dht_temperature = 0.0f;
    dht_result_t dht_result = DHT_RESULT_TIMEOUT;

    // Variáveis para controle do piscar dos LEDs
    bool green_led_on = true;
    bool red_led_on = true;
    absolute_time_t last_led_toggle = get_absolute_time();
    absolute_time_t last_internal_sensor_read = get_absolute_time();
    absolute_time_t last_dht_sensor_read = get_absolute_time();

    // Intervalos de tempo para as operações (em milissegundos)
    const int led_green_interval_ms = 1000;
    const int led_red_interval_ms = 250;
    const int internal_sensor_interval_ms = 1000;
    const int dht_sensor_interval_ms = 2000;  // DHT22 precisa de pelo menos 2s entre leituras

    // Variáveis para armazenar os valores RGB e status
    uint8_t r = 0, g = 0, b = 0;
    int status = 0;

    // Loop principal
    while (true) {
        // Leitura dos botões para alternar entre telas
        bool current_a = gpio_get(BUTTON_A_PIN);
        bool current_b = gpio_get(BUTTON_B_PIN);

        // Detectar pressionamento (borda de descida)
        if (!current_a && last_a_state) {
            screen_mode = (screen_mode + 1) % 4;  // 4 telas disponíveis
            printf("Botão A pressionado, tela: %d\n", screen_mode);
        }
        if (!current_b && last_b_state) {
            screen_mode = (screen_mode - 1 + 4) % 4;  // 4 telas disponíveis
            printf("Botão B pressionado, tela: %d\n", screen_mode);
        }

        // Atualiza o estado anterior dos botões
        last_a_state = current_a;
        last_b_state = current_b;

        // Atualiza temperatura interna a cada 1s
        if (absolute_time_diff_us(last_internal_sensor_read, get_absolute_time()) >= internal_sensor_interval_ms * 1000) {
            // Lê o valor do ADC e converte para temperatura
            float raw_temp = adc_to_temperature(adc_read());
            // Aplica o filtro EMA para suavizar as leituras
            filtered_temp = apply_ema(filtered_temp, raw_temp);

            // Atualiza a variável global para o servidor web
            g_internal_temp_c = filtered_temp;

            // Atualiza histórico de temperatura interna
            for (int i = 0; i < GRAPH_WIDTH - 1; i++) {
                temp_history[i] = temp_history[i + 1];
            }
            temp_history[GRAPH_WIDTH - 1] = filtered_temp;

            // Define a cor do LED com base na temperatura
            temperature_to_rgb(filtered_temp, &r, &g, &b, &status);

            // Atualiza o tempo da última leitura
            last_internal_sensor_read = get_absolute_time();
        }

        // Leitura do sensor DHT22 a cada 2s
        if (absolute_time_diff_us(last_dht_sensor_read, get_absolute_time()) >= dht_sensor_interval_ms * 1000) {
            dht_start_measurement(&dht);
            dht_result = dht_finish_measurement_blocking(&dht, &dht_humidity, &dht_temperature);
            
            // Atualiza as variáveis globais para o servidor web
            if (dht_result == DHT_RESULT_OK) {
                g_external_temp_c = dht_temperature;
                g_humidity = dht_humidity;
                g_dht_status = 1;
                printf("DHT22: %.1f C, %.1f%% humidity\n", dht_temperature, dht_humidity);
                
                // Atualiza histórico de temperatura externa e umidade
                for (int i = 0; i < GRAPH_WIDTH - 1; i++) {
                    ext_temp_history[i] = ext_temp_history[i + 1];
                    humidity_history[i] = humidity_history[i + 1];
                }
                ext_temp_history[GRAPH_WIDTH - 1] = dht_temperature;
                humidity_history[GRAPH_WIDTH - 1] = dht_humidity;
            } else {
                g_dht_status = 0;
                if (dht_result == DHT_RESULT_TIMEOUT) {
                    printf("DHT sensor not responding. Please check your wiring.\n");
                } else {
                    printf("DHT bad checksum\n");
                }
            }
            
            last_dht_sensor_read = get_absolute_time();
        }

        // Prepara as strings para exibição no OLED
        char str_int_temp[20];
        char str_ext_temp[20];
        char str_humidity[20];
        char str_wifi_info[20] = "";
        
        // Temperatura interna do RP2040
        snprintf(str_int_temp, sizeof(str_int_temp), "Int: %.1f C", filtered_temp);
        
        // Temperatura externa do DHT22 (se disponível)
        if (dht_result == DHT_RESULT_OK) {
            snprintf(str_ext_temp, sizeof(str_ext_temp), "Ext: %.1f C", dht_temperature);
            snprintf(str_humidity, sizeof(str_humidity), "Umid: %.1f%%", dht_humidity);
        } else {
            snprintf(str_ext_temp, sizeof(str_ext_temp), "Ext: ---");
            snprintf(str_humidity, sizeof(str_humidity), "Umid: ---");
        }
        
        // Informação de IP se o Wi-Fi estiver conectado
        if (cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA) == CYW43_LINK_UP) {
            uint8_t *ip = (uint8_t *)&cyw43_state.netif[0].ip_addr.addr;
            snprintf(str_wifi_info, sizeof(str_wifi_info), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
        } else {
            snprintf(str_wifi_info, sizeof(str_wifi_info), "WiFi: desconectado");
        }

        // Limpa o buffer do display
        memset(ssd, 0, ssd1306_buffer_length);

        // Exibe diferentes telas com base no modo selecionado
        if (screen_mode == 0) {
            // Tela 1: Informações básicas
            ssd1306_draw_string(ssd, 0, 0, "SENSOR DE TEMP.");
            ssd1306_draw_string(ssd, 0, 16, str_int_temp);
            ssd1306_draw_string(ssd, 0, 32, str_ext_temp);
            ssd1306_draw_string(ssd, 0, 48, str_humidity);
        } else if (screen_mode == 1) {
            // Tela 2: Histórico de temperatura interna
            ssd1306_draw_string(ssd, 0, 0, "Grafico Temp Int");
            for (int i = 0; i < GRAPH_WIDTH; i++) {
                int bar_height = (int)(temp_history[i] * 1.5f);
                if (bar_height > 63) bar_height = 63;
                if (bar_height < 0) bar_height = 0;
                ssd1306_draw_line(ssd, i, 63, i, 63 - bar_height, true);
            }
        } else if (screen_mode == 2) {
            // Tela 3: Histórico de temperatura externa
            ssd1306_draw_string(ssd, 0, 0, "Grafico Temp Ext");
            if (g_dht_status) {
                for (int i = 0; i < GRAPH_WIDTH; i++) {
                    int bar_height = (int)(ext_temp_history[i] * 1.5f);
                    if (bar_height > 63) bar_height = 63;
                    if (bar_height < 0) bar_height = 0;
                    ssd1306_draw_line(ssd, i, 63, i, 63 - bar_height, true);
                }
            } else {
                ssd1306_draw_string(ssd, 0, 32, "O sensor esta");
                ssd1306_draw_string(ssd, 0, 40, "indisponivel");
            }
        } else if (screen_mode == 3) {
            // Tela 4: Histórico de umidade ou Informações WiFi
            if (g_dht_status) {
                ssd1306_draw_string(ssd, 0, 0, "Grafico Umidade");
                for (int i = 0; i < GRAPH_WIDTH; i++) {
                    int bar_height = (int)(humidity_history[i] * 0.63f);
                    if (bar_height > 63) bar_height = 63;
                    if (bar_height < 0) bar_height = 0;
                    ssd1306_draw_line(ssd, i, 63, i, 63 - bar_height, true);
                }
            } else {
                ssd1306_draw_string(ssd, 0, 0, "INFO WIFI");
                ssd1306_draw_string(ssd, 0, 16, WIFI_SSID);
                ssd1306_draw_string(ssd, 0, 32, "IP:");
                ssd1306_draw_string(ssd, 0, 48, str_wifi_info);
            }
        }

        // Envia o buffer para o display
        render_on_display(ssd, &frame_area);

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

    // Nunca deve chegar aqui, mas por completude:
    cyw43_arch_deinit();
    return 0;
}