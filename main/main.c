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

#define DEVICE_ID_SIZE 25

const char *TAG = "OTA_UPDATER";

void app_main(void)
{
    esp_err_t err;

    // just for precaution if some weird bug happens then we at least have one ota partion marked as valid
    err = esp_ota_mark_app_valid_cancel_rollback();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_ota_mark_app_valid_cancel_rollback failed (%s)!", esp_err_to_name(err));
        // you can make it can either restart esp or do some other things like warn the server if wanted
        // cause this error indicates there something corrupeted or worng with the ota partitions ota_data
    }

    init_nvs();
    char *ssid_buf = malloc(WIFI_KEY_SIZE);
    char *pass_buf = malloc(WIFI_KEY_SIZE);
    char *device_id_buf = malloc(DEVICE_ID_SIZE);
    int found_wifi_flag = get_wifi_id_nvs(&ssid_buf, WIFI_KEY_SIZE, &pass_buf, WIFI_KEY_SIZE, &device_id_buf, DEVICE_ID_SIZE);
    if (found_wifi_flag == 0)
    {
        ESP_LOGW(TAG, "WiFi credentials found in NVS");
        ESP_LOGI(TAG, "SSID: %s", ssid_buf);
        ESP_LOGI(TAG, "Password: %s", pass_buf);
        ESP_LOGI(TAG, "Device ID: %s", device_id_buf);
    }
    else if (found_wifi_flag == -1)
    {
        ESP_LOGW(TAG, "WiFi credentials NOT found in NVS");
        ESP_LOGW(TAG, "Will set the device credentials for the first time in NVS");
        set_device_creds_nvs();
        int found_wifi_flag = get_wifi_id_nvs(&ssid_buf, WIFI_KEY_SIZE, &pass_buf, WIFI_KEY_SIZE, &device_id_buf, DEVICE_ID_SIZE);
        if (found_wifi_flag != 0)
        {
            ESP_LOGE(TAG, "Failed to get credentials from NVS after just setting them");
            task_fatal_error();
        }
        ESP_LOGW(TAG, "WiFi credentials found in NVS (we have just set them,this is first boot)");
        ESP_LOGI(TAG, "SSID: %s", ssid_buf);
        ESP_LOGI(TAG, "Password: %s", pass_buf);
        ESP_LOGI(TAG, "Device ID: %s", device_id_buf);

    }
    else
    {
        ESP_LOGE(TAG, "Failed to get WiFi credentials from NVS");
        // unrecoverable error, restart the esp32
        task_fatal_error();
    }

    wifi_init_sta(ssid_buf, pass_buf);
    free(ssid_buf);
    free(pass_buf);
    print_stack_size();
    // 
    // for retriving auth data from nvs we need to allocate memory for the buffers first
    char *cert_buf = malloc(CLIENT_CERT_BUF_SIZE);
    char *key_buf = malloc(KEY_BUF_SIZE);

    int ret = get_auth_nvs(&key_buf, KEY_BUF_SIZE, &cert_buf, CSR_BUF_SIZE);
    if (ret == 0)
    {
        ESP_LOGI(TAG, "Successfully retrieved cert and priv key from NVS");
    }
    else
    {
        // If desired, we can implement different behaviors for when the data is not found or when there is an error in the NVS.
        // For example, if the data is not found, we can create the data, and if there is an error, we can simply restart the ESP32 and try again.
        // The specific behavior depends on the design case.
        // Currently, we have the same behavior for both cases: printing the error, freeing the allocated memory, generating new keys, and storing them in the NVS.
        ESP_LOGE(TAG, "Failed to retrieve cert and priv key from NVS");

        // because it failed we can delete the buffers and generate new ones(the generate_auth_stuff function will allocate memory for the buffers for you this next time)
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

        ESP_LOGI(TAG, "Sucessfully generated csr and priv key key!");

        err = send_csr(csr_buf, &cert_buf, device_id_buf);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to send csr to server and get cert, %s", esp_err_to_name(err));
            // unrecoverable error, restart the esp32
            task_fatal_error();
        }
        free(csr_buf);
        free(device_id_buf);
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

    /*
    We are going now to compare the version of the current firmware with the version of the firmware on the server.
    In this case we are just updating if it is a newer version on the server. This means if you rollback a version on the server the esp32 wont update to the older version.
    */

    char *version_buf2 = NULL;

    char *version_buf1 = calloc(1, VERSION_BUF_SIZE);
    int found_version_flag = get_version_from_nvs(&version_buf1, VERSION_BUF_SIZE);

    int ver_comp_result = -1; // this means if we dont find any version on nvs or get an error retrieving it fomr nvs we will update the ota by default
    char *url_buf = NULL;
    if (found_version_flag == 0)
    {
        ESP_LOGI(TAG, "Version in NVS (current version): %s", version_buf1);
        err = get_version_api(cert_buf, key_buf, &version_buf2, &url_buf);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to get version from API: %s", esp_err_to_name(err));
            // unrecoverable error, restart the esp32
            task_fatal_error();
        }
        ESP_LOGI(TAG, "successfully got data from API");
        ESP_LOGI(TAG, "(server)Version: %s", version_buf2);
        ESP_LOGI(TAG, "URL: %s", url_buf);
        ver_comp_result = compare_versions(version_buf1, version_buf2); // will return -1 if current version is older then server version
        if (ver_comp_result == 0 || ver_comp_result == 1)
        {
            ESP_LOGI(TAG, "Current version is the same or newer then the server version-> no need to update ota!");
        }
    } // first boot probably we dont have the version in the nvs-> still need to get the url from server
    else if (found_version_flag == -1)
    {

        err = get_version_api(cert_buf, key_buf, &version_buf2, &url_buf);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to get version from API");
            // unrecoverable error, restart the esp32
            task_fatal_error();
        }
        ESP_LOGI(TAG, "successfully got data from API");
        ESP_LOGI(TAG, "(server)Version: %s", version_buf2);
        ESP_LOGI(TAG, "URL: %s", url_buf);
    }

    ota_config_t ota_config;
    ota_begin(&ota_config);
    if (ver_comp_result == -1 && url_buf != NULL && version_buf2 != NULL)
    {
        ESP_LOGI(TAG, "settign version in nvs %s", version_buf2);
        err = set_version_in_nvs(version_buf2);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to store version in NVS,%s", esp_err_to_name(err));
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
    free(version_buf2);

    free(url_buf);
    free(cert_buf);
    free(key_buf);
    ota_end(&ota_config);
    ESP_LOGI(TAG, "Everything was excuted successfully!");
    ESP_LOGI(TAG, "Prepare to restart system!");
    esp_restart();
}