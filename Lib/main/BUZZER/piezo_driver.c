#include "piezo_driver.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "../NVS/NVSDriver.h"
#include "../include/Common.h"

uint32_t level=0;

void piezo_init(gpio_num_t gpio_num, uint32_t freq) 
{
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = PIEZO_MODE,
        .timer_num        = PIEZO_TIMER,
        .duty_resolution  = PIEZO_DUTY_RES,
        .freq_hz          = freq,  
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_channel = {
        .speed_mode     = PIEZO_MODE,
        .channel        = PIEZO_CHANNEL,
        .timer_sel      = PIEZO_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = PIEZO_OUTPUT,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}
void piezo_play(uint32_t freq, uint32_t volume)
{
    piezo_init(PIEZO_OUTPUT,freq);
    // Set duty(volume)to 50%. ((2 ** 13) - 1) * 50% = 4095
    ESP_ERROR_CHECK(ledc_set_duty(PIEZO_MODE, PIEZO_CHANNEL, volume));
    ESP_ERROR_CHECK(ledc_update_duty(PIEZO_MODE, PIEZO_CHANNEL));
}
void piezo_stop(void)
{
    ESP_ERROR_CHECK(ledc_set_duty(PIEZO_MODE, PIEZO_CHANNEL, 0));
    ESP_ERROR_CHECK(ledc_update_duty(PIEZO_MODE, PIEZO_CHANNEL));
}

void piezo_set_level(const cJSON * const root)
{
    int array_size = cJSON_GetArraySize(root);
    ESP_LOGI("piezo_set_level", "array_size=%d", array_size);
    uint32_t buzzer_value[array_size];
    for (int i=0; i<array_size; i++) {
        cJSON *array = cJSON_GetArrayItem(root, i);
        buzzer_value[i] = array->valueint;
        ESP_LOGI("piezo_set_level", "buzzer_value[%d]=%d", i, buzzer_value[i]);
    }
    level = (uint32_t)((buzzer_value[0]*8191)/50);

    NVSDriverOpen(NVS_NAMESPACE_CONFIG);
    NVSDriverWriteU32(NVS_KEY_BUZZER_LEVEL, level);
    NVSDriverClose();

    piezo_play(NOTE_C8, level);
    vTaskDelay(100/portTICK_PERIOD_MS);
    piezo_stop();
}

void piezo_set_band(const cJSON * const root)
{
    int array_size = cJSON_GetArraySize(root);
    ESP_LOGI("piezo_set_level", "array_size=%d", array_size);
    uint32_t band_value[array_size];
    for (int i=0; i<array_size; i++) {
        cJSON *array = cJSON_GetArrayItem(root, i);
        band_value[i] = array->valueint;
        ESP_LOGI("piezo_set_level", "band_value[%d]=%d", i, band_value[i]);
    }

    NVSDriverOpen(NVS_NAMESPACE_CONFIG);
    NVSDriverWriteU32(NVS_KEY_BUZZER_BAND, band_value[1]);
    NVSDriverClose();

    piezo_play(band_value[1], level);
    vTaskDelay(100/portTICK_PERIOD_MS);
    piezo_stop();
}
