#include "nvs.h"

void init_nvs(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "nvs init failed (%s)", esp_err_to_name(err));
        task_fatal_error();
    }
}


int get_wifi_id_nvs(char **ssid_buf, size_t ssid_buf_len, char **pass_buf, size_t pass_buf_len, char **device_id_buf, size_t device_id_buf_len)
{
    int toReturn = 1; // 0 if success, -1 if not found, 1 if error
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("device_creds", NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        goto cleanup;
    }

    // Read the SSID from NVS
    size_t required_size;
    err = nvs_get_str(my_handle, "ssid", NULL, &required_size);
    if (err == ESP_OK && required_size <= ssid_buf_len)
    {
        err = nvs_get_str(my_handle, "ssid", *ssid_buf, &required_size);
        if (err == ESP_OK)
        {
            ESP_LOGI(TAG, "SSID retrieved from NVS");
            toReturn = 0;
        }
        else
        {
            ESP_LOGE(TAG, "Failed to read SSID: %s", esp_err_to_name(err));
            goto cleanup;
        }
    }
    else if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGW(TAG, "SSID not found in NVS");
        toReturn = -1;
        goto cleanup;
    }
    else
    {
        ESP_LOGE(TAG, "Failed to get SSID size: %s", esp_err_to_name(err));
        goto cleanup;
    }

    // Read the password from NVS
    size_t required_size2;
    err = nvs_get_str(my_handle, "pass", NULL, &required_size2);
    if (err == ESP_OK && required_size2 <= pass_buf_len)
    {
        err = nvs_get_str(my_handle, "pass", *pass_buf, &required_size2);
        if (err == ESP_OK)
        {
            ESP_LOGI(TAG, "Password retrieved from NVS");
            toReturn = 0;
        }
        else
        {
            ESP_LOGE(TAG, "Failed to read password: %s", esp_err_to_name(err));
            goto cleanup;
        }
    }
    else if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGW(TAG, "Password not found in NVS");
        toReturn = -1;
        goto cleanup;
    }
    else
    {
        ESP_LOGE(TAG, "Failed to get password size: %s", esp_err_to_name(err));
        goto cleanup;
    }

    // Read the device ID from NVS
    size_t required_size3;
    err = nvs_get_str(my_handle, "deviceid", NULL, &required_size3);
    if (err == ESP_OK && required_size3 <= device_id_buf_len)
    {
        err = nvs_get_str(my_handle, "deviceid", *device_id_buf, &required_size3);
        if (err == ESP_OK)
        {
            ESP_LOGI(TAG, "Device ID retrieved from NVS");
            toReturn = 0;
        }
        else
        {
            ESP_LOGE(TAG, "Failed to read device ID: %s", esp_err_to_name(err));
            goto cleanup;
        }
    }
    else if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGW(TAG, "Device ID not found in NVS");
        toReturn = -1;
        goto cleanup;
    }
    else
    {
        ESP_LOGE(TAG, "Failed to get device ID size: %s", esp_err_to_name(err));
        goto cleanup;
    }

cleanup:
    nvs_close(my_handle);
    return toReturn;
}



int get_auth_nvs(char **key_buf, size_t key_buf_len, char **cert_buf, size_t cert_buf_len)
{
    int toReturn = 1; // 0 if success, -1 if not found, 1 if error
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("mtls_auth", NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        goto cleanup;
    }

    // Read the private key from NVS
    size_t required_size;
    err = nvs_get_str(my_handle, "private_key", NULL, &required_size);
    if (err == ESP_OK && required_size <= key_buf_len)
    {
        err = nvs_get_str(my_handle, "private_key", *key_buf, &required_size);
        if (err == ESP_OK)
        {
            ESP_LOGI(TAG, "Private key retrieved from NVS");
            toReturn = 0;
        }
        else
        {
            ESP_LOGE(TAG, "Failed to read private key: %s", esp_err_to_name(err));
            goto cleanup;
        }
    }
    else if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGW(TAG, "Private key not found in NVS");
        toReturn = -1;
        goto cleanup;
    }
    else
    {
        ESP_LOGE(TAG, "Failed to get private key size: %s", esp_err_to_name(err));
        goto cleanup;
    }

    // Read the private key from cert
    size_t required_size2;
    err = nvs_get_str(my_handle, "cert", NULL, &required_size2);
    if (err == ESP_OK && required_size2 <= cert_buf_len)
    {
        err = nvs_get_str(my_handle, "cert", *cert_buf, &required_size2);
        if (err == ESP_OK)
        {
            ESP_LOGI(TAG, "Certificate retrieved from NVS");
            toReturn = 0;
        }
        else
        {
            ESP_LOGE(TAG, "Failed to read certificate: %s", esp_err_to_name(err));
            goto cleanup;
        }
    }
    else if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGW(TAG, "Certificate not found in NVS");
        toReturn = -1;
        goto cleanup;
    }
    else
    {
        ESP_LOGE(TAG, "Failed to get certificate size: %s", esp_err_to_name(err));
        goto cleanup;
    }

    // Commit written value.
    // After setting any values, nvs_commit() must be called to ensure changes are written
    // to flash storage. Implementations may write to storage at other times,
    // but this is not guaranteed.
    ESP_LOGI(TAG, "Committing updates in NVS ... ");
    err = nvs_commit(my_handle);
    if(err != ESP_OK){
        ESP_LOGE(TAG, "Error (%s) committing updates to NVS!\n", esp_err_to_name(err));
        goto cleanup;
    }
    // Close
    
cleanup:
    nvs_close(my_handle);
    return toReturn;
}

esp_err_t set_device_creds_nvs()
{
    print_stack_size();
    esp_err_t err;

    nvs_handle_t my_handle;
    err = nvs_open("device_creds", NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        goto cleanup;
    }
    err = nvs_set_str(my_handle, "ssid", (char*)wifissid_start);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to write wifissid key to NVS");
        goto cleanup;
    }
    err = nvs_set_str(my_handle, "pass", (char*)wifipass_start);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to write wifi pass to NVS");
        goto cleanup;
    }
    err = nvs_set_str(my_handle, "deviceid", (char*)deviceid_start);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to write deviceid to NVS");
        goto cleanup;
    }
    err = nvs_commit(my_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error (%s) committing updates to NVS!\n", esp_err_to_name(err));
        goto cleanup;
    }

cleanup:
    nvs_close(my_handle);
    return err;
}

esp_err_t set_auth_nvs(char *cert_buf, char *key_buf)
{
    print_stack_size();
    esp_err_t err;

    nvs_handle_t my_handle;
    err = nvs_open("mtls_auth", NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        goto cleanup;
    }
    err = nvs_set_str(my_handle, "private_key", key_buf);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to write private key to NVS");
        goto cleanup;
    }
    err = nvs_set_str(my_handle, "cert", cert_buf);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to write certificate to NVS");
        goto cleanup;
    }
    err = nvs_commit(my_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error (%s) committing updates to NVS!\n", esp_err_to_name(err));
        goto cleanup;
    }

cleanup:
    nvs_close(my_handle);
    return err;
}

int get_version_from_nvs(char **version_buf, size_t version_buf_len)
{
    int toReturn = 1; // 0 if success, 1 if error, -1 if not found
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("mtls_auth", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error opening NVS for read: %s", esp_err_to_name(err));
        goto cleanup;
    }

    size_t required_size;
    err = nvs_get_str(nvs_handle, "version", NULL, &required_size);
    if (err == ESP_OK && required_size < version_buf_len)
    {
        err = nvs_get_str(nvs_handle, "version", *version_buf, &required_size);
        if(err == ESP_OK){
            ESP_LOGI(TAG, "Version retrieved from NVS: %s", *version_buf);
            toReturn = 0;
            goto cleanup;
        }
    }
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        // Version not found in NVS
        ESP_LOGE(TAG, "Version not found in NVS");
        toReturn = -1;
        goto cleanup;
    }
    else
    {
        ESP_LOGE(TAG, "Failed to get version size or buffer too small");
        err = ESP_FAIL;
        goto cleanup;
    }
cleanup:
    nvs_close(nvs_handle);
    return toReturn;
}

// Function to set the version in NVS
esp_err_t set_version_in_nvs(const char *version)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("mtls_auth", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error opening NVS for write: %s", esp_err_to_name(err));
        goto cleanup;
    }

    err = nvs_set_str(nvs_handle, "version", version);
    if (err == ESP_OK)
    {
        err = nvs_commit(nvs_handle);
    }
cleanup:
    nvs_close(nvs_handle);
    return err;
}