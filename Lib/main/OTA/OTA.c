#include "OTA.h"
#include "../MQTT/aws.h"
#include "../certs/cert.h"
#include "../include/Common.h"
// #include "../LED/LED.h"
static const char * TAG = "OTAWidget";

char URL[256]="URL";
TaskHandle_t OTATaskHandle;
esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGI(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
    }
    return ESP_OK;
}

void OTATask(void *arg)
{
    ESP_LOGI(TAG, "Stoping AWS Task");
    AWSStop();
    vTaskDelete(SensorTaskHandle);
    vTaskDelete(LTRTaskHandle);

    // SetHS(AMBI_SECONDARY_HUE, AMBI_SECONDARY_SAT);
    // SetBlink();
    
    ESP_LOGI(TAG, "Starting OTA");
    ESP_LOGI(TAG, "FIRMWARE UPGRADE URL: %s", URL);

    esp_http_client_config_t config = {
        .url =URL,
        .cert_pem = ROOT_CA,
        .timeout_ms = 15000,
        .event_handler = _http_event_handler,
        .keep_alive_enable = true,
    };
    while (1) {
        esp_err_t ret = esp_https_ota(&config);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Firmware upgrade success");
            esp_restart();
        } else {
            ESP_LOGE(TAG, "Firmware upgrade failed");
            AWSRun();
            vTaskDelete(OTATaskHandle);
        }
    }
}
void OTARun(const cJSON * const root){
    char *FWURL=cJSON_GetObjectItem(root,"body")->valuestring;
    ESP_LOGI(TAG, "FIRMWARE UPGRADE URL: %s", FWURL);

    sprintf(URL, "%s", FWURL);
    ESP_LOGI(TAG, "FIRMWARE UPGRADE URL: %s", URL);
    free(FWURL);

    ESP_LOGI(TAG, "Stoping AWS Task");
    AWSStop();
    vTaskDelete(SensorTaskHandle);
    vTaskDelete(LTRTaskHandle);
    
    // SetHS(AMBI_SECONDARY_HUE, AMBI_SECONDARY_SAT);
    // SetBlink();
    
    ESP_LOGI(TAG, "Starting OTA");
    ESP_LOGI(TAG, "FIRMWARE UPGRADE URL: %s", URL);
    
    esp_http_client_config_t config = {
        .url =URL,
        .cert_pem = ROOT_CA,
        .timeout_ms = 15000,
        .event_handler = _http_event_handler,
        .keep_alive_enable = true,
    };
    esp_err_t ret = esp_https_ota(&config);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Firmware upgrade success");
        esp_restart();
    } else {
        ESP_LOGE(TAG, "Firmware upgrade failed");
        AWSRun();
    }
}