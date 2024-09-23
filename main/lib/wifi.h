#ifndef MYLIBWIFI_H
#define MYLIBWIFI_H


#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "errno.h"
#include "esp_wifi.h"
#include "esp_eap_client.h"
#include "common.h"



//for normal wifi networks
#define WIFI_SSID "luque"
#define WIFI_PASS "12345678"
#define WIFI_MAXIMUM_RETRY 5




// if using enterprise wifi network-> (for example eduroam) uncomment the following lines
#define USEEAP
#define EAP_SSID
#define EAP_ID "ls-246123@rwu.de"
#define EAP_USERNAME "ls-246123@rwu.de"
#define EAP_PASSWORD "Zvv5N5g6"



// Function to initialize the wifi and sets to station mode
// This function should be called before any other wifi function
void wifi_init_sta(void);

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data);














#endif 