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
#include "lib/ota.h"
#include "lib/https.h"

const char *TAG = "OTA_UPDATER";

void app_main(void)
{
    esp_err_t err;

    // just for precaution if some weird bug happens then we at least have one ota partion marked as valid
    err = esp_ota_mark_app_valid_cancel_rollback();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_ota_mark_app_valid_cancel_rollback failed (%s)!", esp_err_to_name(err));
        // u can make it can either restart esp or do some other things like warn the server if wanted
        // cause this error indicates there something corrupeted or worng with the ota partitions ota_data
    }

    init_nvs();
    wifi_init_sta();
    print_stack_size();
    char *cert_buf = malloc(CLIENT_CERT_BUF_SIZE);
    char *key_buf = malloc(KEY_BUF_SIZE);
    int ret = get_auth_nvs(&key_buf, KEY_BUF_SIZE, &cert_buf, CSR_BUF_SIZE);
    if (ret == 0)
    {
        ESP_LOGI(TAG, "Successfully retrieved cert and priv key from NVS");
    }
    else
    {
        // TODO! if wanted we can have different behavior if the data is not found or if there is an error in the nvs
        // for example, if data was not found we create the data and if there is an error we just restart the esp32 and try again
        //  depends on the desing case
        //  for now we have the same behavior for both cases: print the error, free the allocated memory, try to generate new keys and store them in the nvs
        ESP_LOGE(TAG, "Failed to retrieve cert and priv key from NVS");
        free(cert_buf);
        free(key_buf);

        char *csr_buf = NULL;

        err = generate_auth_stuff(&csr_buf, &key_buf);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to generate csr and priv key, %s", esp_err_to_name(err));
            // unrecoverable error, restart the esp32
            task_fatal_error();
        }
        // TODO! remove print certificate and key
        ESP_LOGI(TAG, "Sucessfully generated csr and priv key key!");

        err = send_csr(csr_buf, &cert_buf);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to send csr to server and get cert, %s", esp_err_to_name(err));
            // unrecoverable error, restart the esp32
            task_fatal_error();
        }
        free(csr_buf);
        ESP_LOGI(TAG, "Successfully got certificate from server with csr");
        err = set_auth_nvs(cert_buf, key_buf);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to store auth data in NVS, %s", esp_err_to_name(err));
            // unrecoverable error, restart the esp32
            task_fatal_error();
        }
        ESP_LOGI(TAG, "Successfully stored cert and priv key in NVS");
    }
    char *version_buf1 = malloc(VERSION_BUF_SIZE);
    int found_version_flag = get_version_from_nvs(&version_buf1, VERSION_BUF_SIZE);

    int ver_comp_result = -1; // this means if we dont find any version on nvs or get an error retrieving it fomr nvs we will update the ota by default
    char *url_buf = NULL;
    if (found_version_flag == 0)
    {
        ESP_LOGI(TAG, "Version in NVS (current version): %s", version_buf1);
        char *version_buf2 = NULL;
        err = get_version_api(cert_buf, key_buf, &version_buf2, &url_buf);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to get version from API: %s", esp_err_to_name(err));
            // unrecoverable error, restart the esp32
            task_fatal_error();
        }
        ESP_LOGI(TAG, "successfully got version from API");
        ESP_LOGI(TAG, "Version: %s", version_buf2);
        ESP_LOGI(TAG, "URL: %s", url_buf);
        ver_comp_result = compare_versions(version_buf1, version_buf2); // will return -1 if current version is older then server version
        if(ver_comp_result == 0 || ver_comp_result == 1){
            ESP_LOGI(TAG, "Current version is the same or newer then the server version-> no need to update ota!");
        }
        free(version_buf2);

    } // first boot probably we dont have the version in the nvs-> still need to get the url from server
    else if (found_version_flag == -1)
    {
        char *version_buf2 = malloc(VERSION_BUF_SIZE);

        err = get_version_api(cert_buf, key_buf, &version_buf2, &url_buf);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to get version from API");
            // unrecoverable error, restart the esp32
            task_fatal_error();
        }
        ESP_LOGI(TAG, "successfully got version from API");
        ESP_LOGI(TAG, "Version: %s", version_buf2);
        ESP_LOGI(TAG, "URL: %s", url_buf);
    }

    ota_config_t ota_config;
    ota_begin(&ota_config);
    if (ver_comp_result == -1 && url_buf != NULL)
    {
        err = set_version_in_nvs(version_buf1);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to store version in NVS,%s" , esp_err_to_name(err));
            ESP_LOGW(TAG, "Will continue with the update process anyways");
        }
        else
        {
            ESP_LOGI(TAG, "Successfully stored version in NVS");
        }

        ESP_LOGI(TAG, "Current version is older than server version-> will update ota!");
        err = ota_update(cert_buf, key_buf, url_buf, &ota_config);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to to donwload or update ota");
            // unrecoverable error, restart the esp32
            task_fatal_error();
        }
    }
    free(version_buf1);
    free(url_buf);
    free(cert_buf);
    free(key_buf);
    ota_end(&ota_config);
    ESP_LOGI(TAG, "Everything was excuted successfully!");
    ESP_LOGI(TAG, "Prepare to restart system!");
    esp_restart();
}