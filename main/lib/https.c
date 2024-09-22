#include "https.h"






esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static char *output_buffer;  // Buffer to store response of http request from event handler
    static int output_len;       // Stores number of bytes read
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            // Clean the buffer in case of a new request
            if (output_len == 0 && evt->user_data) {
                // we are just starting to copy the output data into the use
                memset(evt->user_data, 0, MAX_HTTP_OUTPUT_BUFFER);
            }
            /*
             *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
             *  However, event handler can also be used in case chunked encoding is used.
             */
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // If user_data buffer is configured, copy the response into the buffer
                int copy_len = 0;
                if (evt->user_data) {
                    // The last byte in evt->user_data is kept for the NULL character in case of out-of-bound access.
                    copy_len = MIN(evt->data_len, (MAX_HTTP_OUTPUT_BUFFER - output_len));
                    if (copy_len) {
                        memcpy(evt->user_data + output_len, evt->data, copy_len);
                    }
                } else {
                    int content_len = esp_http_client_get_content_length(evt->client);
                    if (output_buffer == NULL) {
                        // We initialize output_buffer with 0 because it is used by strlen() and similar functions therefore should be null terminated.
                        output_buffer = (char *) calloc(content_len + 1, sizeof(char));
                        output_len = 0;
                        if (output_buffer == NULL) {
                            ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                            return ESP_FAIL;
                        }
                    }
                    copy_len = MIN(evt->data_len, (content_len - output_len));
                    if (copy_len) {
                        memcpy(output_buffer + output_len, evt->data, copy_len);
                    }
                }
                output_len += copy_len;
            }

            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            if (output_buffer != NULL) {
                // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
                // ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            int mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
            if (err != 0) {
                ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
                ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
            }
            if (output_buffer != NULL) {
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
            esp_http_client_set_header(evt->client, "From", "user@example.com");
            esp_http_client_set_header(evt->client, "Accept", "text/html");
            esp_http_client_set_redirection(evt->client);
            break;
    }
    return ESP_OK;
}


esp_err_t get_version_api(char* cert_buf, char* key_buf, char** version_buf, char** url_buf)
{
    // Declare local_response_buffer with size (MAX_HTTP_OUTPUT_BUFFER + 1) to prevent out of bound access when
    // it is used by functions like strlen(). The buffer should only be used upto size MAX_HTTP_OUTPUT_BUFFER
    char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER + 1] = {0};
    
    print_stack_size();

    esp_http_client_config_t config = {

        #ifdef TESTSERVER
        .url= "https://192.168.247.220/",
        #endif
        #ifndef TESTSERVER
        .url = GET_VERSION_URL,
        #endif
        .event_handler = _http_event_handler,
        //.crt_bundle_attach = esp_crt_bundle_attach,
        
        .user_data = local_response_buffer,        // Pass address of local buffer to get response
        .crt_bundle_attach = esp_crt_bundle_attach,
        //.cert_len = server_cert_pem_end - server_cert_pem_start,
        .client_cert_pem = (char *)cert_buf,
        .client_key_pem = (char *)key_buf,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);

    // GET
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        int content_length = esp_http_client_get_content_length(client);
        
        ESP_LOGI(TAG, "HTTPS Status = %d, content_length = %d", status_code, content_length);
        if (status_code != 200) 
        {
            err = ESP_FAIL;
            ESP_LOGE(TAG, "HTTP GET INVALID CODE");
            ESP_LOGE(TAG,"Response: %s",local_response_buffer);
            goto cleanup;
        }
        if (local_response_buffer[0] != '\0') {
            ESP_LOGI(TAG, "Response: %s", local_response_buffer);
            cJSON *root = cJSON_Parse(local_response_buffer);
            if (root != NULL) {
                char *json_string = cJSON_Print(root);
                ESP_LOGI(TAG, "Parsed JSON response: %s", json_string);
                
                // Extract a version field
                cJSON *result = cJSON_GetObjectItemCaseSensitive(root, "version");
                if (cJSON_IsString(result) && (result->valuestring != NULL)) {
                    size_t version_len = strlen(result->valuestring);
                    if (version_len < VERSION_BUF_SIZE) {
                        *version_buf = (char *)malloc((version_len + 1) * sizeof(char));
                        if (*version_buf != NULL) {
                            strncpy(*version_buf, result->valuestring, version_len);
                            (*version_buf)[version_len] = '\0';
                            ESP_LOGI(TAG, "Version: %s", *version_buf);
                        } else {
                            ESP_LOGE(TAG, "Failed to allocate memory for version buffer");
                            err = ESP_ERR_NO_MEM;
                            goto cleanupjson;
                        }
                    } else {
                        ESP_LOGE(TAG, "Version string length exceeds buffer size");
                        err = ESP_ERR_NO_MEM;
                        goto cleanupjson;
                    }
                }

                // Extract a url field
                cJSON *result2 = cJSON_GetObjectItemCaseSensitive(root, "url");
                if (cJSON_IsString(result2) && (result2->valuestring != NULL)) {
                    size_t url_len = strlen(result2->valuestring);
                    if (url_len < URL_BUF_SIZE) {
                        *url_buf = (char *)malloc((url_len + 1) * sizeof(char));
                        if (*url_buf != NULL) {
                            strncpy(*url_buf, result2->valuestring, url_len);
                            (*url_buf)[url_len] = '\0';
                            ESP_LOGI(TAG, "URL: %s", *url_buf);
                        } else {
                            ESP_LOGE(TAG, "Failed to allocate memory for url buffer");
                            err = ESP_ERR_NO_MEM;
                            goto cleanupjson;
                        }
                    } else {
                        ESP_LOGE(TAG, "url length exceeds buffer size");
                        err = ESP_ERR_NO_MEM;
                        goto cleanupjson;
                    }
                }
            cleanupjson:
                free(json_string);
                cJSON_Delete(root);
            } else {
                ESP_LOGE(TAG, "Failed to parse JSON response");
                err = ESP_FAIL;
            }
        } else {
            ESP_LOGE(TAG, "Response buffer is empty");
            err = ESP_FAIL;
        }

    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }
    
cleanup:
    esp_http_client_cleanup(client);
    return err;
}



esp_err_t send_csr(const char *csr, char **cert_buf)
{
    char *local_response_buffer = (char *)calloc(MAX_HTTP_OUTPUT_BUFFER + 1, sizeof(char));
    if (local_response_buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for local response buffer");
        return ESP_ERR_NO_MEM;
    }


    esp_http_client_config_t config = {
        .url = GET_CRT_URL,
        .event_handler = _http_event_handler,
        .user_data = local_response_buffer,        // Pass address of local buffer to get response
        .crt_bundle_attach = esp_crt_bundle_attach,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "deviceId", DEVICE_ID);
    cJSON_AddStringToObject(root, "csr", csr);
    const char *post_data = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    printf("POST DATA: %s\n", post_data);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));    

    esp_err_t err = esp_http_client_perform(client);
    free(post_data);
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        int content_length = esp_http_client_get_content_length(client);
        
        ESP_LOGI(TAG, "HTTPS Status = %d, content_length = %d", status_code, content_length);
        if (status_code != 200) 
        {
            err = ESP_FAIL;
            ESP_LOGE(TAG, "HTTP POST INVALID CODE");
            ESP_LOGE(TAG, "%s",local_response_buffer);

            goto cleanuphttps;
        }
        
        if (local_response_buffer[0] != '\0') {
            cJSON *root = cJSON_Parse(local_response_buffer);
            if (root != NULL) {
                char *json_string = cJSON_Print(root);
                ESP_LOGI(TAG, "Parsed JSON response: %s", json_string);
                
                // Extract a status field
                cJSON *result = cJSON_GetObjectItemCaseSensitive(root, "status");
                if (cJSON_IsString(result) && (result->valuestring != NULL)) {
                    ESP_LOGI(TAG, "Result: %s", result->valuestring);
                    if (strcmp(result->valuestring, "success") != 0)
                    {
                        err = ESP_FAIL;
                        goto cleanupjson;
                    }
                    
                }

                // Extract a certificate field
                cJSON *result2 = cJSON_GetObjectItemCaseSensitive(root, "certificate");
                if (cJSON_IsString(result2) && (result2->valuestring != NULL)) {
                    size_t cert_len = strlen(result2->valuestring);
                    if (cert_len < CLIENT_CERT_BUF_SIZE) {
                        *cert_buf = (char *)malloc((cert_len + 1) * sizeof(char));
                        if (*cert_buf != NULL) {
                            strncpy(*cert_buf, result2->valuestring, cert_len);
                            (*cert_buf)[cert_len] = '\0';
                        } else {
                            ESP_LOGE(TAG, "Failed to allocate memory for certificate buffer");
                            err = ESP_ERR_NO_MEM;
                        }
                    } else {
                        ESP_LOGE(TAG, "Certificate length exceeds buffer size");
                        err = ESP_ERR_NO_MEM;
                    }
                }
            cleanupjson:
                free(json_string);
                cJSON_Delete(root);
            } else {
                ESP_LOGE(TAG, "Failed to parse JSON response");
                err = ESP_FAIL;
            }
        } else {
            ESP_LOGE(TAG, "Response buffer is empty");
            err = ESP_FAIL;
        }
    } else {
        ESP_LOGE(TAG, "HTTPS POST request failed: %s", esp_err_to_name(err));
    }
cleanuphttps:
    free(local_response_buffer);
    esp_http_client_cleanup(client);    
    return err;
}
