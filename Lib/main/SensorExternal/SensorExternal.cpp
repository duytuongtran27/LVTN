#include "SensorExternal.h"

#include "hw_config.h"
#include "Common.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "stdio.h"
#include "stdbool.h"
#include "stdint.h"
#include "stddef.h"

#include <app-common/zap-generated/attributes/Accessors.h>
#include <common/Esp32AppServer.h>
#include <platform/CHIPDeviceLayer.h>

static const char * TAG = "SensorExternal";

static sht4x_t dev1;
static ltr303_t dev2;
static scd4x_t dev3;

using namespace ::chip;
using namespace ::chip::DeviceLayer;

SensorData_t SensorData;

TaskHandle_t SensorTaskHandle;
TaskHandle_t LTRTaskHandle;

void sensor_task(void *arg)
{
    //ESP_ERROR_CHECK(i2cdev_init());

    memset(&dev1, 0, sizeof(sht4x_t));
    ESP_ERROR_CHECK(sht4x_init_desc(&dev1, 0, I2C_SDA, I2C_SCL));
    ESP_ERROR_CHECK(sht4x_init(&dev1));

    vTaskDelay(40000/portTICK_PERIOD_MS);

    memset(&dev3, 0, sizeof(scd4x_t));
    ESP_ERROR_CHECK(scd4x_init_desc(&dev3, 0, I2C_SDA, I2C_SCL));
    ESP_LOGI(TAG, "Initializing sensor...");
    scd4x_reinit(&dev3);
    ESP_LOGI(TAG, "Sensor initialized");

    scd4x_start_periodic_measurement(&dev3);
    ESP_LOGI(TAG, "Periodic measurements started");

    while (1)
    {
        ESP_LOGI(TAG, "Stack remaining for task '%s' is %d bytes", pcTaskGetName(SensorTaskHandle), uxTaskGetStackHighWaterMark(SensorTaskHandle));
        esp_err_t res = scd4x_read_measurement(&dev3, &SensorData.co2, &SensorData.temperature_scd, &SensorData.humidity_scd);
        if (res != ESP_OK)
        {
            ESP_LOGE(TAG, "Error reading results %d (%s)", res, esp_err_to_name(res));
            continue;
        }

        if (SensorData.co2 == 0)
        {
            ESP_LOGW(TAG, "Invalid sample detected, skipping");
            continue;
        }

        ESP_LOGI(TAG, "Measure Value of SCD40: CO2 = %u ppm, Temperature = %.2f °C, Humidity: %.2f %%", SensorData.co2, SensorData.temperature_scd, SensorData.humidity_scd);

        vTaskDelay(1000/portTICK_PERIOD_MS);

        res = sht4x_measure(&dev1, &SensorData.temperature_sht, &SensorData.humidity_sht);
        if (res != ESP_OK)
        {
            ESP_LOGE(TAG, "Error reading results %d (%s)", res, esp_err_to_name(res));
            continue;
        }
        ESP_LOGI(TAG,"Measure Value of SHT40: Temperature = %.2f °C, Humidity = %.2f %%", SensorData.temperature_sht, SensorData.humidity_sht);

        chip::DeviceLayer::PlatformMgr().LockChipStack();
        chip::app::Clusters::TemperatureMeasurement::Attributes::MeasuredValue::Set(1, static_cast<int16_t>(SensorData.temperature_sht * 100));
        chip::app::Clusters::RelativeHumidityMeasurement::Attributes::MeasuredValue::Set(2, static_cast<int16_t>(SensorData.humidity_sht * 100));
        chip::DeviceLayer::PlatformMgr().UnlockChipStack();

        vTaskDelay(29000/portTICK_PERIOD_MS);
    }
}
void ltr303_task(void *arg){
    
    memset(&dev2,0, sizeof(ltr303_t));
    ESP_ERROR_CHECK(ltr303_init_desc(&dev2, 0, I2C_SDA, I2C_SCL));
    ltr303_init(&dev2);
    vTaskDelay(40000/portTICK_PERIOD_MS);
    while (1)
    {
        ESP_LOGI(TAG, "Stack remaining for task '%s' is %d bytes", pcTaskGetName(LTRTaskHandle), uxTaskGetStackHighWaterMark(LTRTaskHandle));
        ltr303_get_raw_data(&dev2);
        ltr303_get_lux_data(&dev2,&SensorData.lux, &SensorData.fspec, &SensorData.inspec);

        ESP_LOGI(TAG,"Measure Value of LTR303: Lux = %.2f, Full Spectrum = %u, Infrared Spectrum = %u", SensorData.lux, SensorData.fspec, SensorData.inspec);

        wifi_ap_record_t ap_info;
        esp_err_t err;

        err = esp_wifi_sta_get_ap_info(&ap_info);

        if (err == ESP_OK)
        {
            int16_t rssi=0;
            rssi = ap_info.rssi;
            if (rssi >= -50) {
                SensorData.signal_quality = 100;
            } else if (rssi >= -80) { // between -50 ~ -80dbm
                SensorData.signal_quality = (uint8_t)(24 + (rssi + 80) * 2.6);
            } else if (rssi >= -90) { // between -80 ~ -90dbm
                SensorData.signal_quality = (uint8_t)((rssi + 90) * 2.6);
            } else { // < -84 dbm
                SensorData.signal_quality = 0;
            }
        }
        chip::DeviceLayer::PlatformMgr().LockChipStack();
        chip::app::Clusters::IlluminanceMeasurement::Attributes::MeasuredValue::Set(3, static_cast<int16_t>(SensorData.lux * 100));
        chip::DeviceLayer::PlatformMgr().UnlockChipStack();
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
}
void SensorExternalRun(){
    xTaskCreate(&sensor_task, "sensor_task", 3072, NULL, 10, &SensorTaskHandle);
    xTaskCreate(&ltr303_task, "ltr303_task", 2560, NULL, 10, &LTRTaskHandle);
}