#ifndef MYLIBHELPERS_H
#define MYLIBHELPERS_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "errno.h"
#include "esp_system.h"
#include "esp_event.h"
#include "common.h"
#include "esp_ota_ops.h"
#include "esp_app_format.h"

// This is just for debugging purposes
static void print_stack_size()
{
    ESP_LOGI(TAG, "Available stack size: %d bytes", uxTaskGetStackHighWaterMark(NULL) * sizeof(StackType_t));
}



static void __attribute__((noreturn)) task_fatal_error(void)
{
    esp_err_t err;
    ESP_LOGE(TAG, "Exiting task due to fatal error...");

    const esp_partition_t *running = esp_ota_get_running_partition();
    err = esp_ota_set_boot_partition(running);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
        ESP_LOGE(TAG, "restarting system and try again");
    }

    esp_restart();
    
    (void)vTaskDelete(NULL);
}

// Function to compare two version strings
// Returns 1 if v1 is newer, -1 if v1 is older, and 0 if they are equal
// the first version is the subject and the second is the target of comparation
static int compare_versions(const char *v1, const char *v2) {
    int v1_major, v1_minor, v1_patch;
    int v2_major, v2_minor, v2_patch;

    // Parse the version strings into major, minor, and patch integers
    sscanf(v1, "%d.%d.%d", &v1_major, &v1_minor, &v1_patch);
    sscanf(v2, "%d.%d.%d", &v2_major, &v2_minor, &v2_patch);

    // Compare major version
    if (v1_major > v2_major) return 1;
    if (v1_major < v2_major) return -1;

    // Compare minor version
    if (v1_minor > v2_minor) return 1;
    if (v1_minor < v2_minor) return -1;

    // Compare patch version
    if (v1_patch > v2_patch) return 1;
    if (v1_patch < v2_patch) return -1;

    // Versions are equal
    return 0;
}




#endif 