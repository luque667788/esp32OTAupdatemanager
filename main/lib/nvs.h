#ifndef MYLIBNVS_H
#define MYLIBNVS_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "errno.h"
#include "esp_system.h"
#include "esp_event.h"
#include "helpers.h"

#include "common.h"

extern const uint8_t wifissid_start[] asm("_binary_wifissid_start");
extern const uint8_t wifissid_end[] asm("_binary_wifissid_end");

extern const uint8_t wifipass_start[] asm("_binary_wifipass_start");
extern const uint8_t wifipass_end[] asm("_binary_wifipass_end");

extern const uint8_t deviceid_start[] asm("_binary_deviceid_start");
extern const uint8_t deviceid_end[] asm("_binary_deviceid_end");


// Function to initialize the NVS
// This function should be called before any other NVS function or wifi
void init_nvs(void);


// Get the WiFi ID (SSID, password, and device ID) from the NVS
// Returns 0 if success, -1 if not found, 1 if error
// The ssid_buf, pass_buf, and device_id_buf should be pre-allocated before calling this function
int get_wifi_id_nvs(char **ssid_buf, size_t ssid_buf_len, char **pass_buf, size_t pass_buf_len, char **device_id_buf, size_t device_id_buf_len);


// Get the authentication data (priv key and cert) from the NVS
// Returns 0 if success, -1 if not found, 1 if error
// The key_buf and cert_buf should be pre-allocated before calling this function
int get_auth_nvs(char **key_buf, size_t key_buf_len, char **cert_buf, size_t cert_buf_len);

// Set the authentication data (priv key and cert) in the NVS
// does not take ownership of the buffers(copies the data)
esp_err_t set_auth_nvs(char *cert_buf, char *key_buf);

// Get the version from the NVS
// Returns 0 if success, 1 if error, -1 if not found
// The version_buf should be pre-allocated before calling this function
int get_version_from_nvs(char **version_buf, size_t version_buf_len);

// Set the version in the NVS
// does not take ownership of the buffers(copies the data)
esp_err_t set_version_in_nvs(const char *version);

esp_err_t set_device_creds_nvs();


#endif