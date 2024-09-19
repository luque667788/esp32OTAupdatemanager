#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "esp_ota_ops.h"
#include "esp_app_format.h"
#include "esp_flash_partitions.h"

#include "esp_partition.h"
#include "lib/wifi.h"
#include "lib/helpers.h"
#include "lib/nvs.h"
#include "lib/gen_auth.h"

const char *TAG = "OTA_UPDATER";






void app_main(void)
{
    init_nvs();
    wifi_init_sta();
    print_stack_size();
    esp_err_t err;
    char *csr_buf = malloc(CSR_BUF_SIZE);
    char *key_buf = malloc(KEY_BUF_SIZE);
    int ret = get_auth_nvs(&key_buf,KEY_BUF_SIZE, &csr_buf, CSR_BUF_SIZE);
    if(ret == 0){
        ESP_LOGI(TAG, "Successfully retrieved auth data from NVS");
        //TODO! remove print certificate and key
        ESP_LOGI(TAG, "Key: %s", key_buf);
        ESP_LOGI(TAG, "Cert: %s", csr_buf);
    }else{
        //TODO! if wanted we can have different behavior if the data is not found or if there is an error in the nvs
        //for example, if data was not found we create the data and if there is an error we just restart the esp32 and try again
        // depends on the desing case
        // for now we have the same behavior for both cases: print the error, free the allocated memory, try to generate new keys and store them in the nvs
        ESP_LOGE(TAG, "Failed to retrieve auth data from NVS");
        free(csr_buf);
        free(key_buf);
        
        
        err = generate_auth_stuff(&csr_buf, &key_buf);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to generate auth data");
            // unrecoverable error, restart the esp32
            task_fatal_error();
        }
        //TODO! remove print certificate and key
        ESP_LOGI(TAG, "Key: %s", key_buf);
        ESP_LOGI(TAG, "Cert: %s", csr_buf);
        err = set_auth_nvs(csr_buf, key_buf);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to store auth data in NVS");
            // unrecoverable error, restart the esp32
            task_fatal_error();
        }



    }

}