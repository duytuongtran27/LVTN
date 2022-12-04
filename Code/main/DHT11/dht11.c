#include "dht11.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"


static const char *DHT11_TAG = "DHT11";

void dht11_init() { gpio_pad_select_gpio(DHT11_PIN); }

static bool dht11_start() {
    gpio_set_direction(DHT11_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT11_PIN, 0); 
    ets_delay_us(19 * 1000);      
    gpio_set_level(DHT11_PIN, 1); 
    ets_delay_us(30);             

    gpio_set_direction(DHT11_PIN, GPIO_MODE_INPUT);
    int64_t timestamp_us = esp_timer_get_time();
    while (!gpio_get_level(DHT11_PIN)) { 
        if (esp_timer_get_time() - timestamp_us > 100)
            return false;
    }

    timestamp_us = esp_timer_get_time();
    while (gpio_get_level(DHT11_PIN)) {
        if (esp_timer_get_time() - timestamp_us > 100)
            return false;
    }

    return true;
}

static bool dht11_read_byte(uint8_t *value) {
    if (value == NULL) {
        return false;
    }

    *value = 0;
    for (int i = 8; i > 0; i--) {
        *value <<= 1;

        int64_t timestamp_us = esp_timer_get_time();
        while (!gpio_get_level(DHT11_PIN)) {
            if (esp_timer_get_time() - timestamp_us > 70)
                return false;
        }

        ets_delay_us(40); 
        if (gpio_get_level(DHT11_PIN)) {
            *value |= 1;
        }
        timestamp_us = esp_timer_get_time();
        while (gpio_get_level(DHT11_PIN)) {
            if (esp_timer_get_time() - timestamp_us > 50)
                return false;
        }
    }
    return true;
}

bool dht11_read(dht11_data_t *dht11_data) {
    if (dht11_data == NULL) {
        ESP_LOGE(DHT11_TAG, "dht11_data is NULL!");
        return false;
    }

    uint8_t buf[4] = {0};
    uint8_t checksum;

    if (!dht11_start()) {
        ESP_LOGE(DHT11_TAG, "No responses from DHT11!");
        return false;
    }

    if (!(dht11_read_byte(&buf[0]) && dht11_read_byte(&buf[1]) && dht11_read_byte(&buf[2]) &&
          dht11_read_byte(&buf[3]) && dht11_read_byte(&checksum))) {
        ESP_LOGE(DHT11_TAG, "Failed to read data from DHT11!");
        return false;
    }

    if (checksum != buf[0] + buf[1] + buf[2] + buf[3]) { 
        ESP_LOGE(DHT11_TAG, "Data from DHT11 is invalid!");
        return false;
    }

    dht11_data->temperature = buf[2];
    dht11_data->humidity = buf[0];
    ESP_LOGI(DHT11_TAG, "Temperature: %d Humidity: %d", dht11_data->temperature, dht11_data->humidity);
    return true;
}
