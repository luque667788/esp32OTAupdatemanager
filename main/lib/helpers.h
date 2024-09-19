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

static void print_stack_size()
{
    ESP_LOGI(TAG, "Available stack size: %d bytes", uxTaskGetStackHighWaterMark(NULL) * sizeof(StackType_t));
}



static void __attribute__((noreturn)) task_fatal_error(void)
{
    //esp_err_t err;
    ESP_LOGE(TAG, "Exiting task due to fatal error...");
/*
    const esp_partition_t *running = esp_ota_get_running_partition();
    err = esp_ota_set_boot_partition(running);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
        ESP_LOGE(TAG, "restarting system try again");
    }

    // esp_restart();
    */
    (void)vTaskDelete(NULL);
}




#endif 