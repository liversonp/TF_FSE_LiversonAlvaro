#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define LED 2

void app_main(void)
{
    esp_rom_gpio_pad_select_gpio(LED);
    gpio_set_direction(LED, GPIO_MODE_OUTPUT);

    int estado = 0;
    while (true)
    {
        gpio_set_level(LED, estado);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        estado = !estado;
    }
}
