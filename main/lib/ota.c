#include "ota.h"


static void http_cleanup(esp_http_client_handle_t client)
{
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
}


void ota_begin(ota_config_t *ota_config){
    ota_config->update_handle = 0;
    ota_config->update_partition = esp_ota_get_next_update_partition(NULL);
    if (ota_config->update_partition == NULL)
    {
        ESP_LOGE(TAG, "Application Partition not found");
        //unrecoverable error, restart the esp32
        task_fatal_error();
    }
    ota_config->running_partition = esp_ota_get_running_partition();
    if (ota_config->running_partition == NULL)
    {
        ESP_LOGE(TAG, "Running Partition not found");
        //unrecoverable error, restart the esp32
        task_fatal_error();
    }
    if (ota_config->update_partition == ota_config->running_partition)
    {
        ESP_LOGW(TAG, "Running partition is the same as the update partition probably a bug in previous versons of application bin");
        ESP_LOGW(TAG, "will continue update process anyways");
    }
}

static char ota_write_data[OTA_BUFFSIZE + 1] = { 0 };
esp_err_t ota_update(char* cert_buf,char* key_buf,char* url_buf,ota_config_t *ota_config)
{
    esp_err_t err;

    esp_http_client_config_t config = {
        .url = (char *)url_buf,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .client_cert_pem = (char *)cert_buf,
        .client_key_pem = (char *)key_buf,
        .timeout_ms = OTA_RECV_TIMEOUT,
        .keep_alive_enable = true,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL)
    {
        ESP_LOGE(TAG, "Failed to initialise HTTP connection");
        task_fatal_error();
    }
    err = esp_http_client_open(client, 0);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        task_fatal_error();
    }
    esp_http_client_fetch_headers(client);
    int binary_file_length = 0;

    bool image_header_was_checked = false;

    while (1)
    {
        // reads the data from the server and writes to the ota partition in chunks of size BUFFSIZE
        int data_read = esp_http_client_read(client, ota_write_data, OTA_BUFFSIZE);
        if (data_read < 0)
        {
            ESP_LOGE(TAG, "Error: SSL data read error");
            http_cleanup(client);
            task_fatal_error();
        }
        else if (data_read > 0)
        {
            if (image_header_was_checked == false)
            {
                // will check the header of the image to compare versions and also call the esp_ota_begin function but only in the first iteration
                esp_app_desc_t new_app_info;
                if (data_read > sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t))
                {
                    // check current version with downloading
                    memcpy(&new_app_info, &ota_write_data[sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)], sizeof(esp_app_desc_t));

                    image_header_was_checked = true;

                    err = esp_ota_begin(ota_config->update_partition, OTA_WITH_SEQUENTIAL_WRITES, &ota_config->update_handle);
                    if (err != ESP_OK)
                    {
                        ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
                        http_cleanup(client);
                        esp_ota_abort(ota_config->update_handle);
                        task_fatal_error();
                    }
                    ESP_LOGI(TAG, "esp_ota_begin succeeded");
                }
                else
                {
                    ESP_LOGE(TAG, "first received package is not fit len (too small)");
                    http_cleanup(client);
                    esp_ota_abort(ota_config->update_handle);
                    task_fatal_error();
                }
            }
            err = esp_ota_write(ota_config->update_handle, (const void *)ota_write_data, data_read);
            if (err != ESP_OK)
            {
                http_cleanup(client);
                esp_ota_abort(ota_config->update_handle);
                task_fatal_error();
            }
            binary_file_length += data_read;
            ESP_LOGD(TAG, "Written image length %d", binary_file_length);
        }
        else if (data_read == 0)
        {
            /*
             * As esp_http_client_read never returns negative error code, we rely on
             * `errno` to check for underlying transport connectivity closure if any
             */
            if (errno == ECONNRESET || errno == ENOTCONN)
            {
                ESP_LOGE(TAG, "Connection closed, errno = %d", errno);
                break;
            }
            if (esp_http_client_is_complete_data_received(client) == true)
            {
                ESP_LOGI(TAG, "Connection closed");
                break;
            }
        }
    }
    ESP_LOGI(TAG, "Total Write binary data length: %d", binary_file_length);
    if (esp_http_client_is_complete_data_received(client) != true)
    {
        ESP_LOGE(TAG, "Error in receiving complete file");
        http_cleanup(client);
        esp_ota_abort(ota_config->update_handle);
        task_fatal_error();
    }

    err = esp_ota_end(ota_config->update_handle);
    if (err != ESP_OK)
    {
        if (err == ESP_ERR_OTA_VALIDATE_FAILED)
        {
            ESP_LOGE(TAG, "Image validation failed, image is corrupted");
        }
        else
        {
            ESP_LOGE(TAG, "esp_ota_end failed (%s)!", esp_err_to_name(err));
        }
        http_cleanup(client);
        task_fatal_error();
    }
    return ESP_OK;
    

}


esp_err_t ota_end(ota_config_t *ota_config){
    esp_err_t err;
    err = esp_ota_set_boot_partition(ota_config->update_partition);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
        task_fatal_error();
    }
    return err;
}


