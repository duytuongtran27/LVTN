#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"

#define LED_GPIO 4

static const char *TAG = "Led";

esp_err_t Led_init(){
    gpio_pad_select_gpio(LED_GPIO);

    if (gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT) != ESP_OK){
        ESP_LOGE(TAG, "GPIO init error");
        return ESP_ERR_INVALID_ARG;
    }
    ESP_LOGI(TAG, "Led init successfully");
    return ESP_OK;
}

esp_err_t set_led_level(uint32_t level)
{
    if (gpio_set_level(LED_GPIO, level) != ESP_OK){
        ESP_LOGE(TAG, "set GPIO error");
        return ESP_ERR_INVALID_ARG;
    }
    ESP_LOGI(TAG, "Led state: %d", level);
    return ESP_OK;
}

esp_err_t reset_led_pin(){
    gpio_reset_pin(LED_GPIO);
    ESP_LOGI(TAG, "led reseted");
    return ESP_OK;
}