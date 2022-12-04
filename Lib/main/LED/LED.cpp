#include "LED.h"
#include "Common.h"
#include "../NVS/NVSDriver.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"

typedef enum {
	LED_IDLE = 0, 
	LED_SOLID = 1, 
	LED_BLINK = 2, 
	LED_FADE = 3, 
	LED_FADEOUT = 4, 
	LED_FADEIN = 5,
    LED_BREATHE = 6
} LED_Mode_t;

typedef struct
{
    uint8_t mode;
    bool state;
    uint8_t brightness;
    uint8_t brightnessStable;
    uint8_t hue;
    uint8_t hueStable;
    uint8_t saturation;
    uint8_t saturationStable;
    uint32_t BlinkTime;
} LED_t;

LED_t LedParam;

TaskHandle_t LEDTaskHandle;

void LED::Init(void)
{
    mState = false;
    mDefaultOnBrightness = UINT8_MAX;
    mHue = 0;
    mSaturation = 0;
    ws2812_spi_init(0, LED_DATA, LED_NUMBER);
}

void LED::Set(bool state)
{
    LedParam.mode = LED_SOLID;
    mState = state;
    LedParam.state = mState;
}

void LED::Blink(uint32_t blinkTimeMS)
{
    LedParam.mode = LED_BLINK;
    LedParam.BlinkTime = blinkTimeMS;
}

void LED::FadeIn(void)
{
    LedParam.mode = LED_BREATHE;
}

void LED::SetLedBrightness(const cJSON * const root)
{
    int array_size = cJSON_GetArraySize(root);
    ESP_LOGI("SetLedBrightness", "array_size=%d", array_size);
    uint32_t led_value[array_size];
    for (int i=0; i<array_size; i++) {
        cJSON *array = cJSON_GetArrayItem(root, i);
        led_value[i] = array->valueint;
        ESP_LOGI("SetLedBrightness", "ir_value[%d]=%d", i, led_value[i]);
    }

    mLedLevel = ((led_value[1] * 255)/10000);

    NVSDriverOpen(NVS_NAMESPACE_CONFIG);
    NVSDriverWriteU32(NVS_KEY_LED_LEVEL, mLedLevel);
    NVSDriverClose();

    SetBrightness(mLedLevel);
}

void LED::SetBrightness(uint8_t brightness)
{
    HsvColor_t hsv = {mHue, mSaturation, brightness};
    RgbColor_t rgb = HsvToRgb(hsv);

    ESP_LOGI("SetColor", "red %d, green %d, blue %d", rgb.r, rgb.g, rgb.b);
    ws2812_spi_set(LED_INDEX_0, rgb.r, rgb.g, rgb.b);
    ws2812_spi_refresh(LED_TIME_REFRESH);
    if (brightness > 0)
    {
        mDefaultOnBrightness = brightness;
    }
    UpdateStableState();
}
void LED::SetColor(uint8_t Hue, uint8_t Saturation)
{
    uint8_t brightness = mState ? mDefaultOnBrightness : 0;
    mHue = Hue;
    mSaturation = Saturation;

    HsvColor_t hsv = {mHue, mSaturation, brightness};
    RgbColor_t rgb = HsvToRgb(hsv);

    ESP_LOGI("SetColor", "red %d, green %d, blue %d", rgb.r, rgb.g, rgb.b);
    ws2812_spi_set(LED_INDEX_0, rgb.r, rgb.g, rgb.b);
    ws2812_spi_refresh(LED_TIME_REFRESH);
    UpdateStableState();
}
void LED::UpdateStableState(void)
{
    LedParam.hueStable = mHue;
    LedParam.saturationStable = mSaturation;
    LedParam.brightnessStable = mDefaultOnBrightness;
}
void LEDTask(void *arg)
{
    while (true)
    {
        LED_t *recv = (LED_t *)arg;
        switch (recv->mode)
        {
            case LED_SOLID:
            {
                //printf("mode:%d\n", recv->mode);
                uint8_t brightness = recv->state ? recv->brightnessStable : 0;

                HsvColor_t hsv = {recv->hueStable, recv->saturationStable, brightness};
                RgbColor_t rgb = HsvToRgb(hsv);

                ESP_LOGD("SetColor", "red %d, green %d, blue %d", rgb.r, rgb.g, rgb.b);
                ws2812_spi_set(LED_INDEX_0, rgb.r, rgb.g, rgb.b);
                ws2812_spi_refresh(LED_TIME_REFRESH);
                recv->mode = LED_IDLE;
            }
            break;

            case LED_BLINK:
            {
                uint8_t state = recv->state ^ 1;
                uint8_t brightness = state ? recv->brightnessStable : 0;

                HsvColor_t hsv = {recv->hueStable, recv->saturationStable, brightness};
                RgbColor_t rgb = HsvToRgb(hsv);

                ESP_LOGD("SetColor", "red %d, green %d, blue %d", rgb.r, rgb.g, rgb.b);
                ws2812_spi_set(LED_INDEX_0, rgb.r, rgb.g, rgb.b);
                ws2812_spi_refresh(LED_TIME_REFRESH);
                vTaskDelay(recv->BlinkTime / portTICK_PERIOD_MS);
                recv->state = state;
            }
            break;

            case LED_BREATHE:
            {
                for(uint8_t b = 5; b<255; b++){
                    HsvColor_t hsv = {recv->hueStable, recv->saturationStable, b};
                    RgbColor_t rgb = HsvToRgb(hsv);
   
                    ESP_LOGD("SetColor", "red %d, green %d, blue %d", rgb.r, rgb.g, rgb.b);
                    ws2812_spi_set(LED_INDEX_0, rgb.r, rgb.g, rgb.b);
                    ws2812_spi_refresh(LED_TIME_REFRESH);
                    vTaskDelay(10/ portTICK_PERIOD_MS);
                }
                for(uint8_t b = 255; b>5; b--){
                    HsvColor_t hsv = {recv->hueStable, recv->saturationStable, b};
                    RgbColor_t rgb = HsvToRgb(hsv);

                    ESP_LOGD("SetColor", "red %d, green %d, blue %d", rgb.r, rgb.g, rgb.b);
                    ws2812_spi_set(LED_INDEX_0, rgb.r, rgb.g, rgb.b);
                    ws2812_spi_refresh(LED_TIME_REFRESH);
                    vTaskDelay(10 / portTICK_PERIOD_MS);
                }
            }
            break;

            case LED_IDLE:
            {
                vTaskDelay(10 / portTICK_PERIOD_MS);
            }
            break;
        }
        ESP_LOGD("LEDTask", "Stack remaining for task '%s' is %d bytes", pcTaskGetName(LEDTaskHandle), uxTaskGetStackHighWaterMark(LEDTaskHandle));
    }
}
void LED::Run(void)
{
    xTaskCreate(&LEDTask, "LEDTask", 2560, (void *)&LedParam, 11, &LEDTaskHandle);
}
RgbColor_t HsvToRgb(HsvColor_t hsv)
{
    RgbColor_t rgb;

    uint16_t i = hsv.h / 60;
    uint16_t rgb_max = hsv.v;
    uint16_t rgb_min = (uint16_t)(rgb_max * (100 - hsv.s)) / 100;
    uint16_t diff = hsv.h % 60;
    uint16_t rgb_adj = (uint16_t)((rgb_max - rgb_min) * diff) / 60;

    switch (i)
    {
		case 0:
			rgb.r = (uint8_t)rgb_max;
			rgb.g = (uint8_t)(rgb_min + rgb_adj);
			rgb.b = (uint8_t)rgb_min;
			break;
		case 1:
			rgb.r = (uint8_t)(rgb_max - rgb_adj);
			rgb.g = (uint8_t)rgb_max;
			rgb.b = (uint8_t)rgb_min;
			break;
		case 2:
			rgb.r = (uint8_t)rgb_min;
			rgb.g = (uint8_t)rgb_max;
			rgb.b = (uint8_t)(rgb_min + rgb_adj);
			break;
		case 3:
			rgb.r = (uint8_t)rgb_min;
			rgb.g = (uint8_t)(rgb_max - rgb_adj);
			rgb.b = (uint8_t)rgb_max;
			break;
		case 4:
			rgb.r = (uint8_t)(rgb_min + rgb_adj);
			rgb.g = (uint8_t)rgb_min;
			rgb.b = (uint8_t)rgb_max;
			break;
		default:
			rgb.r = (uint8_t)rgb_max;
			rgb.g = (uint8_t)rgb_min;
			rgb.b = (uint8_t)(rgb_max - rgb_adj);
			break;
    }

    return rgb;
}
