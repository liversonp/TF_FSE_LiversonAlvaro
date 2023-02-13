#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"

void nvsInit(){
    // Inicializa o NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

int32_t nvsGetValue(char *vkey){

    ESP_ERROR_CHECK(nvs_flash_init());

    int32_t c_val = 0;
    nvs_handle_t particaoH;

    esp_err_t resp = nvs_open("storageData", NVS_READONLY, &particaoH);

    if (resp != ESP_OK){
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(resp));
    }else{
        esp_err_t gres = nvs_get_i32(particaoH, vkey, &c_val);

        switch (gres){
        case ESP_OK:
            break;
        case ESP_ERR_NOT_FOUND:
            printf("NVS Valor não encontrado");
            return -1;
        default:
            printf("NVS Erro ao acessar o NVS (%s)", esp_err_to_name(gres));
            return -1;
            break;
        }

        nvs_close(particaoH);
    }
    return c_val;
}

void nvsWriteValue(char *vkey, int32_t valor){
    ESP_ERROR_CHECK(nvs_flash_init());

    nvs_handle_t particaoH;

    esp_err_t resp = nvs_open("storageData", NVS_READWRITE, &particaoH);
 
    if (resp != ESP_OK){
        printf("NVS Namespace: storageData, não encontrado");
    }

    else{
        esp_err_t gres = nvs_set_i32(particaoH, vkey, valor);
        if (gres != ESP_OK){
            printf("NVS Não foi possível escrever no NVS (%s)", esp_err_to_name(gres));
        }
        else{
            //ESP_LOGI("NVS", "%s gravado com sucesso", vkey);
        }
        nvs_commit(particaoH);
    }

    nvs_close(particaoH);
}