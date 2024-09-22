#ifndef MYLIBOTA_H
#define MYLIBOTA_H

#include "esp_ota_ops.h"
#include "esp_app_format.h"
#include "esp_flash_partitions.h"
#include "common.h"
#include "esp_partition.h"
#include "helpers.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"

#include "esp_log.h"
#include "errno.h"
#include "esp_system.h"
#include "esp_event.h"



#define OTA_BUFFSIZE 1024
#define OTA_RECV_TIMEOUT 3000
//struct that holds ota config data
typedef struct ota_config_t
{
    esp_ota_handle_t update_handle;
    const esp_partition_t *update_partition;
    const esp_partition_t *running_partition;
} ota_config_t;


// Gets the current partition and the partition to update
//sets up the ota_config_t struct
void ota_begin(ota_config_t *ota_config);

// sets the next boot to be on the application partition
esp_err_t ota_end(ota_config_t *ota_config);

esp_err_t ota_update(char* cert_buf,char* key_buf,char* url_buf,ota_config_t *ota_config);



#endif 