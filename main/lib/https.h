#ifndef MYHTTPS_H
#define MYHTTPS_H

#include "esp_http_client.h"
#include <stdlib.h>
#include "esp_log.h"
#include "errno.h"
#include "esp_tls.h"
#include "helpers.h"
#include <string.h>
#include <sys/param.h>
#include <ctype.h>
#include "esp_system.h"
#include "common.h"
#include "cJSON.h"


#include "esp_sntp.h"
#include "esp_netif.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "esp_tls.h"
#include "sdkconfig.h"
#include "esp_crt_bundle.h"

#include "mbedtls/debug.h"




/**** CONFIGURATION ****/
#define GET_CRT_URL "https://taylered.io/api/device/register"

#define GET_VERSION_URL "https://mtls.taylered.io/api/device/pull/update"

#define DEVICE_ID "lq5jbbj9srauod4"
/****               ****/

#define MAX_HTTP_RECV_BUFFER 512
#define MAX_HTTP_OUTPUT_BUFFER 2048

#ifndef CLIENT_CERT_BUF_SIZE
    #define CLIENT_CERT_BUF_SIZE 2048
#endif


#ifndef URL_BUF_SIZE
    #define URL_BUF_SIZE 100
#endif

#ifndef VERSION_BUF_SIZE
    #define VERSION_BUF_SIZE 100
#endif




// Function to send a CSR to the server and receive a certificate
// The certificate is stored in cert_buf that will be allocated on the HEAP for you in the function
// Returns ESP_OK if successful, ESP_FAIL if not
esp_err_t send_csr(const char *csr, char **cert_buf);

//http handler function -> defined as esp32 http example
esp_err_t _http_event_handler(esp_http_client_event_t *evt);

// Function to get the version from the server
// allocates memory for version_buf and url_buf for you on the HEAP
// returns ESP_OK if successful, ESP_FAIL if not
esp_err_t get_version_api(char* cert_buf, char* key_buf, char** version_buf, char** url_buf);




#endif