/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_log.h"

#include "dht11.h"
#include "app_wifi.h"
#include "led.h"
#include "stream_server.h"
#include "esp_camera.h"
// #include "IRSend.h"
#include "IRSendRaw.h"
#include "IRReceive.h"
#include "rtsp_server.h"

#define DHT11_GPIO 16

#define CAMERA_PIN_PWDN 32
#define CAMERA_PIN_RESET -1
#define CAMERA_PIN_XCLK 0
#define CAMERA_PIN_SIOD 26
#define CAMERA_PIN_SIOC 27

#define CAMERA_PIN_D7 35
#define CAMERA_PIN_D6 34
#define CAMERA_PIN_D5 39
#define CAMERA_PIN_D4 36
#define CAMERA_PIN_D3 21
#define CAMERA_PIN_D2 19
#define CAMERA_PIN_D1 18
#define CAMERA_PIN_D0 5
#define CAMERA_PIN_VSYNC 25
#define CAMERA_PIN_HREF 23
#define CAMERA_PIN_PCLK 22

static const char *TAG = "main code";

#define TEST_ESP_OK(ret) assert(ret == ESP_OK)
#define TEST_ASSERT_NOT_NULL(ret) assert(ret != NULL)

static bool auto_jpeg_support = false; // whether the camera sensor support compression or JPEG encode
static QueueHandle_t xQueueIFrame = NULL;

esp_err_t start_stream_server(const QueueHandle_t frame_i, const bool return_fb);

static esp_err_t init_camera(uint32_t xclk_freq_hz, pixformat_t pixel_format, framesize_t frame_size, uint8_t fb_count)
{
    camera_config_t camera_config = {
        .pin_pwdn = CAMERA_PIN_PWDN,
        .pin_reset = CAMERA_PIN_RESET,
        .pin_xclk = CAMERA_PIN_XCLK,
        .pin_sscb_sda = CAMERA_PIN_SIOD,
        .pin_sscb_scl = CAMERA_PIN_SIOC,

        .pin_d7 = CAMERA_PIN_D7,
        .pin_d6 = CAMERA_PIN_D6,
        .pin_d5 = CAMERA_PIN_D5,
        .pin_d4 = CAMERA_PIN_D4,
        .pin_d3 = CAMERA_PIN_D3,
        .pin_d2 = CAMERA_PIN_D2,
        .pin_d1 = CAMERA_PIN_D1,
        .pin_d0 = CAMERA_PIN_D0,
        .pin_vsync = CAMERA_PIN_VSYNC,
        .pin_href = CAMERA_PIN_HREF,
        .pin_pclk = CAMERA_PIN_PCLK,

        //EXPERIMENTAL: Set to 16MHz on ESP32-S2 or ESP32-S3 to enable EDMA mode
        .xclk_freq_hz = xclk_freq_hz,
        .ledc_timer = LEDC_TIMER_0, // // This is only valid on ESP32/ESP32-S2. ESP32-S3 use LCD_CAM interface.
        .ledc_channel = LEDC_CHANNEL_0,

        .pixel_format = pixel_format, //YUV422,GRAYSCALE,RGB565,JPEG
        .frame_size = frame_size,    //QQVGA-UXGA, sizes above QVGA are not been recommended when not JPEG format.

        .jpeg_quality = 10, //0-63 
        .fb_count = fb_count,       // For ESP32/ESP32-S2, if more than one, i2s runs in continuous mode. Use only with JPEG.
        .grab_mode = CAMERA_GRAB_LATEST,
        .fb_location = CAMERA_FB_IN_PSRAM
    };

    //initialize the camera
    esp_err_t ret = esp_camera_init(&camera_config);

    sensor_t *s = esp_camera_sensor_get();
    s->set_vflip(s, 1);//flip it back
    //initial sensors are flipped vertically and colors are a bit saturated
    if (s->id.PID == OV3660_PID) {
        s->set_saturation(s, -2);//lower the saturation
    }

    if (s->id.PID == OV3660_PID || s->id.PID == OV2640_PID) {
        s->set_vflip(s, 1); //flip it back    
    } else if (s->id.PID == GC0308_PID) {
        s->set_hmirror(s, 0);
    } else if (s->id.PID == GC032A_PID) {
        s->set_vflip(s, 1);
    }

    camera_sensor_info_t *s_info = esp_camera_sensor_get_info(&(s->id));

    if (ESP_OK == ret && PIXFORMAT_JPEG == pixel_format && s_info->support_jpeg == true) {
        auto_jpeg_support = true;
    }

    return ret;
}

void blink_led(){
  Led_init();
  while (1)
  {
    set_led_level(1);
    vTaskDelay(200);
    set_led_level(0);
    vTaskDelay(200);
  }
}

void Read_DHT11(){
  dht11_init();
  dht11_data_t dht11_data;
  while (true) {
      if (dht11_read(&dht11_data)) {
          ESP_LOGI(TAG, "temperature: %d humidity: %d", dht11_data.temperature, dht11_data.humidity);
      }
      vTaskDelay(3000 / portTICK_PERIOD_MS);
  }
}

void app_main(void) 
{
  /* Print chip information */
  esp_chip_info_t chip_info;
  uint32_t flash_size;
  esp_chip_info(&chip_info);
  printf("This is %s chip with %d CPU core(s), WiFi%s%s, ",
          CONFIG_IDF_TARGET,
          chip_info.cores,
          (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
          (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

  printf("silicon revision %d, ", chip_info.revision);
  if(esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
      printf("Get flash size failed");
      return;
  }

  // main code
  //----------init-------------
  //Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  // xTaskCreate(&blink_led, "blink led", 2048, NULL, 2, NULL);
  //xTaskCreate(&Read_DHT11, "read dht11", 2048, NULL, 1, NULL);

  // // //---------------wifi-------------------
  // ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
  // app_wifi_main();

//   //---------camera------------
//   app_wifi_main();
  
//   camera_fb_t *frame;
  
//   xQueueIFrame = xQueueCreate(2, sizeof(camera_fb_t *));

//   /* It is recommended to use a camera sensor with JPEG compression to maximize the speed */
//   TEST_ESP_OK(init_camera(10000000, PIXFORMAT_JPEG, FRAMESIZE_QVGA, 2));
//   TEST_ESP_OK(start_stream_server(xQueueIFrame, true));
//   ESP_LOGI(TAG, "Begin capture frame");
  
//   while (true) {
//       frame = esp_camera_fb_get();
//       if (frame) {
//           xQueueSend(xQueueIFrame, &frame, portMAX_DELAY);
//       } 
//   }

//--------IR Send-------------

// printf("Start rtsp");
//  rtsp_server();
rmt_rx_init();

xTaskCreate(&ir_rx_received, "ir_rx_received", 2048, NULL, 10, NULL);
rmt_tx_init();
for (int i=0; i<5; i++){
  ir_tx_send();
  vTaskDelay(5000/portTICK_PERIOD_MS);
}

}