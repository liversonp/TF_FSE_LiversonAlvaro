#include <stdio.h>
#include <string.h>
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/semphr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "sdkconfig.h"
#include "esp_adc/adc_oneshot.h"

#include "DHT11/dht11.h"
#include "WIFI/wifi.h"
#include "MQTT/mqtt.h"
#include "GPIO/gpio_setup.h"
#include "NVS/treatnvs.h"

// Variables
float temperatura = 0, totalTemp = 0;
float umidade = 0, totalUmid = 0;
int status, miniReed = 1;
int counter = 1;

SemaphoreHandle_t conexaoWifiSemaphore;
SemaphoreHandle_t conexaoMQTTSemaphore;
SemaphoreHandle_t dht11Semaphore;
SemaphoreHandle_t REEDSemaphore;

#define LED 2
#define M_REED 4

void conectadoWifi(void * params)
{
  while(true)
  {
    if(xSemaphoreTake(conexaoWifiSemaphore, portMAX_DELAY))
    {
      // Processamento Internet
      mqtt_start();
    }
  }
}

void initGPIO(){
    // Configuração do Timer
    ledc_timer_config_t timer_config = {
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .duty_resolution = LEDC_TIMER_8_BIT,
      .timer_num = LEDC_TIMER_0,
      .freq_hz = 1000,
      .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer_config);

    // Configuração do Canal
    ledc_channel_config_t channel_config = {
      .gpio_num = 2,
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .channel = LEDC_CHANNEL_0,
      .timer_sel = LEDC_TIMER_0,
      .duty = 0,
      .hpoint = 0
    };
    ledc_channel_config(&channel_config);
    ledc_fade_func_install(0);
}

void trataComunicacaoComServidor(void * params)
{
  char mensagem[100];
  if(xSemaphoreTake(conexaoMQTTSemaphore, portMAX_DELAY))
  {
    int ledPower;
    while(true)
    {
      if(status == 0 && temperatura != 0 && umidade != 0){
        ledPower = nvsGetValue("led");
        sprintf(mensagem, "{ \"temperatura\": \"%.2f\", \"umidade\": \"%.2f\", \"reed\":\"%d\", \"led\": \"%d\" }", temperatura, umidade, miniReed, ledPower);
        mqtt_envia_mensagem("v1/devices/me/telemetry", mensagem);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
      }
    }
  }
}

void trataDht(void *params){
  float temp;
  float umid;
    while(true) {
        temp = DHT11_read().temperature;
        status = DHT11_read().status;
        umid = DHT11_read().humidity;
        if(temp > 1 && umid > 1){
          totalTemp += temp;
          totalUmid += umid;
          temperatura = totalTemp/counter;
          umidade = totalUmid/counter;
          printf("Temperature is %.2f \n", temperatura);
          printf("Humidity is %.2f\n", umidade);
          printf("Status code is %d\n", status);
          vTaskDelay(10000 / portTICK_PERIOD_MS);
          counter++;
        }else{
          status = -1;
        }
    }
}

void trataM_REED(void *params) {
  while(1){
    miniReed = !digitalRead(M_REED);
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

void app_main(void)
{
    // Iniciar LED & Mini Reed
    esp_rom_gpio_pad_select_gpio(LED);
    gpio_set_direction(LED, GPIO_MODE_OUTPUT);

    esp_rom_gpio_pad_select_gpio(M_REED);
    gpio_set_direction(LED, GPIO_MODE_INPUT);

    // Iniciar DHT11
    DHT11_init(GPIO_NUM_16);

    //Inicia NVS
    nvsInit();
    
    //Definir LED no Ultimo valor antes da placa desligar
    initGPIO();
    int ledPower = nvsGetValue("led");
    printf("%d\n", ledPower);
    ledc_set_fade_time_and_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, ledPower, 1000, LEDC_FADE_WAIT_DONE);
    
    // Semaforos
    conexaoWifiSemaphore = xSemaphoreCreateBinary();
    conexaoMQTTSemaphore = xSemaphoreCreateBinary();
    dht11Semaphore = xSemaphoreCreateBinary();
    REEDSemaphore = xSemaphoreCreateBinary();

    wifi_start();

    // Tasks
    xTaskCreate(&conectadoWifi,  "Conexão ao MQTT", 4096, NULL, 1, NULL);
    xTaskCreate(&trataComunicacaoComServidor, "Comunicação com Broker", 4096, NULL, 1, NULL);
    xTaskCreate(&trataDht, "Leitura DHT11", 4096, NULL, 1, NULL);
    xTaskCreate(&trataM_REED, "Leitura KYC", 4096, NULL, 1, NULL);
}