#include <esp_http_server.h>
#include <esp_log.h>
#include <string>
#include <string.h>
#include <iostream>
#include "cJSON.h"

#include "IntexSWG.h"
#include "utils.h"
#include "RestServer.h"

using namespace std;

#define SCRATCH_BUFSIZE (10240)


/* @brief tag used for ESP serial console messages */
static const char TAG[] = "api_rest";

/* @brief the HTTP server handle */
static httpd_handle_t server = NULL;


// HTTP GET General info request
static esp_err_t general_info_get_handler(httpd_req_t *req) {

    httpd_resp_set_type(req, "application/json");
    
    std::string displayDigits;
    displayDigits += getDisplayDigitFromCode(displayingDigit2);
    displayDigits += getDisplayDigitFromCode(displayingDigit1);
    
    cJSON *root = cJSON_CreateObject();
    cJSON *data = cJSON_CreateObject();
    
    cJSON *display = cJSON_CreateObject();
    cJSON_AddStringToObject(display, "status", displayON ? "ON" : "OFF");
    cJSON_AddNumberToObject(display, "brightness", displayIntensity);
    cJSON_AddStringToObject(display, "current_code", displayDigits.c_str());
    cJSON_AddItemToObject(data, "display", display);

    cJSON *status = cJSON_CreateObject();
    cJSON_AddStringToObject(status, "power", (powerStatus == POWER_STATUS_BOOTING) ? "BOOTING" : (powerStatus == POWER_STATUS_ON) ? "ON" : "STANDBY");
    cJSON_AddStringToObject(status, "boost", (statusDigit3 & (0x01 << LED_BOOST)) >> LED_BOOST == 1 ? "ON" : "OFF");
    cJSON_AddStringToObject(status, "sleep", (statusDigit3 & (0x01 << LED_SLEEP)) >> LED_SLEEP == 1 ? "ON" : "OFF");
    cJSON_AddStringToObject(status, "o3_generation", (statusDigit3 & (0x01 << LED_OZONE)) >> LED_OZONE == 1 ? "ON" : "OFF");
    cJSON_AddStringToObject(status, "pump_low_flow", (statusDigit3 & (0x01 << LED_PUMP_LOW_FLOW)) >> LED_PUMP_LOW_FLOW == 1 ? "ON" : "OFF");
    cJSON_AddStringToObject(status, "low_salt", (statusDigit3 & (0x01 << LED_LOW_SALT)) >> LED_LOW_SALT == 1 ? "ON" : "OFF");
    cJSON_AddStringToObject(status, "high_salt", (statusDigit3 & (0x01 << LED_HIGH_SALT)) >> LED_HIGH_SALT == 1 ? "ON" : "OFF");
    cJSON_AddStringToObject(status, "service", (statusDigit3 & (0x01 << LED_SERVICE)) >> LED_SERVICE == 1 ? "ON" : "OFF");
    cJSON_AddItemToObject(data, "status", status);

    cJSON *mode = cJSON_CreateObject();
    cJSON_AddBoolToObject(mode, "working", powerStatus == 1);
    cJSON_AddBoolToObject(mode, "programming", displayBlinking);
    cJSON_AddItemToObject(data, "mode", mode);

    cJSON_AddItemToObject(root, "data", data);
    
    const char *response = cJSON_Print(root);
    httpd_resp_sendstr(req, response);
    free((void *)response);
    cJSON_Delete(root);
    return ESP_OK;
}

/* Simple handler for power control */
static esp_err_t general_info_post_handler(httpd_req_t *req)
{
    bool responseStatus = false;
    int remaining = req->content_len;
    char buffer[100];
    int received = 0;
    if (remaining >= 100) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (remaining > 0) {
        received = httpd_req_recv(req, buffer, remaining);
        if (received <= 0) {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        remaining -= received;
    }

    cJSON *root = cJSON_Parse(buffer);
    cJSON* cjson_data = cJSON_GetObjectItem(root, "data");
    char* power = cJSON_GetObjectItem(cjson_data, "power")->valuestring;

    keyCodeSetByAPI = true;
    virtualPressButtonTime = 250;
    
    if (strcmp(power, "on") == 0) {        
        if (powerStatus == POWER_STATUS_STANDBY) {
            buttonStatus = BUTTON_POWER;   
            responseStatus = true;         
        }
    }
    // TODO: Change when relay is installed
    else if (strcmp(power, "off") == 0) { 
        if (powerStatus == POWER_STATUS_BOOTING || powerStatus == POWER_STATUS_ON) {
            buttonStatus = BUTTON_POWER;
            responseStatus = true;
        }
    }
    else if (strcmp(power, "standby") == 0) {
        if (powerStatus == POWER_STATUS_BOOTING || powerStatus == POWER_STATUS_ON) {
            buttonStatus = BUTTON_POWER;
            responseStatus = true;
        }
    }            

    cJSON_Delete(root);

    // Response
    cJSON *responseRoot = cJSON_CreateObject();
    cJSON *responseData = cJSON_CreateObject();    
    cJSON_AddBoolToObject(responseData, "status", responseStatus);
    cJSON_AddItemToObject(responseRoot, "data", responseData);
    
    const char *response = cJSON_Print(responseRoot);
    httpd_resp_sendstr(req, response);
    free((void *)response);
    cJSON_Delete(responseRoot);

    return ESP_OK;
}

/* Simple handler for display control */
static esp_err_t display_post_handler(httpd_req_t *req)
{
    int remaining = req->content_len;
    char buffer[100];
    int received = 0;
    if (remaining >= 100) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (remaining > 0) {
        received = httpd_req_recv(req, buffer, remaining);
        if (received <= 0) {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        remaining -= received;
    }

    cJSON *root = cJSON_Parse(buffer);
    cJSON* cjson_data = cJSON_GetObjectItem(root, "data");
    displayIntensity = cJSON_GetObjectItem(cjson_data, "brightness")->valueint % 8; 
    ESP_LOGI(TAG, "Set display brightness to '%d'", displayIntensity);     

    cJSON_Delete(root);

    // Response
    cJSON *responseRoot = cJSON_CreateObject();
    cJSON *responseData = cJSON_CreateObject();    
    cJSON_AddBoolToObject(responseData, "status", true);
    cJSON_AddItemToObject(responseRoot, "data", responseData);
    
    const char *response = cJSON_Print(responseRoot);
    httpd_resp_sendstr(req, response);
    free((void *)response);
    cJSON_Delete(responseRoot);

    return ESP_OK;
}

/* Simple handler for display control */
static esp_err_t wifi_config_delete_handler(httpd_req_t *req)
{
    removeWifiConfig = true;

    // Response
    cJSON *responseRoot = cJSON_CreateObject();
    cJSON *responseData = cJSON_CreateObject();    
    cJSON_AddBoolToObject(responseData, "status", true);
    cJSON_AddItemToObject(responseRoot, "data", responseData);

    const char *response = cJSON_Print(responseRoot);
    httpd_resp_sendstr(req, response);
    free((void *)response);
    cJSON_Delete(responseRoot);

    return ESP_OK;
}

static const httpd_uri_t swg_status_get_uri = {
    .uri       = "/api/v1/intex/swg/status",
    .method    = HTTP_GET,
    .handler   = general_info_get_handler,
    .user_ctx  = NULL
};

/* URI handler for power control */
static const httpd_uri_t swg_status_post_uri = {
    .uri = "/api/v1/intex/swg",
    .method = HTTP_POST,
    .handler = general_info_post_handler,
    .user_ctx = NULL
};

/* URI handler for display control */
static const httpd_uri_t display_post_uri = {
    .uri = "/api/v1/intex/swg/display",
    .method = HTTP_POST,
    .handler = display_post_handler,
    .user_ctx = NULL
};

/* URI handler for delete wifi config */
static const httpd_uri_t wifi_config_delete_uri = {
    .uri = "/api/v1/intex/swg/wifi",
    .method = HTTP_DELETE,
    .handler = wifi_config_delete_handler,
    .user_ctx = NULL
};

void start_rest_server(unsigned int port) {
    server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;
    config.server_port = port;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &swg_status_get_uri);
        httpd_register_uri_handler(server, &swg_status_post_uri);
        httpd_register_uri_handler(server, &display_post_uri);
        httpd_register_uri_handler(server, &wifi_config_delete_uri);
    }

    ESP_LOGI(TAG, "Error starting server!"); 
}

void stop_webserver() {
    // Stop the httpd server
    httpd_stop(server);
}
