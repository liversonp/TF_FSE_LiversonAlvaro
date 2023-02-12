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

#include "dht11.h"
#include "wifi.h"
#include "mqtt.h"

// Variables
float temperatura = 0, totalTemp = 0;
float umidade = 0, totalUmid = 0;
int status;
int counter = 1;

SemaphoreHandle_t conexaoWifiSemaphore;
SemaphoreHandle_t conexaoMQTTSemaphore;
SemaphoreHandle_t dht11Semaphore;
SemaphoreHandle_t KYCSemaphore;

#define LED 2
#define KYC 4

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

void trataComunicacaoComServidor(void * params)
{
  char mensagem[50];
  if(xSemaphoreTake(conexaoMQTTSemaphore, portMAX_DELAY))
  {
    while(true)
    {
      if(status == 0 && temperatura != 0 && umidade != 0){
        sprintf(mensagem, "{ \"temperatura\": \"%.2f\", \"umidade\": \"%.2f\"}", temperatura, umidade);
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

void trataKY039(void *params) {
  float pulse = 0;
  while(1){
    for(int i = 0; i < 20; i++){
      pulse += gpio_get_level(KYC);
    }
    printf("%.2f\n", pulse/20);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void app_main(void)
{
    // Inicializa o NVS
    esp_rom_gpio_pad_select_gpio(LED);
    gpio_set_direction(LED, GPIO_MODE_OUTPUT);

    esp_rom_gpio_pad_select_gpio(KYC);
    gpio_set_direction(KYC, GPIO_MODE_INPUT);

    DHT11_init(GPIO_NUM_16);
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    conexaoWifiSemaphore = xSemaphoreCreateBinary();
    conexaoMQTTSemaphore = xSemaphoreCreateBinary();
    dht11Semaphore = xSemaphoreCreateBinary();
    KYCSemaphore = xSemaphoreCreateBinary();

    wifi_start();

    xTaskCreate(&conectadoWifi,  "Conexão ao MQTT", 4096, NULL, 1, NULL);
    xTaskCreate(&trataComunicacaoComServidor, "Comunicação com Broker", 4096, NULL, 1, NULL);
    xTaskCreate(&trataDht, "Leitura DHT11", 4096, NULL, 1, NULL);
    xTaskCreate(&trataKY039, "Leitura KYC", 4096, NULL, 1, NULL);
}
