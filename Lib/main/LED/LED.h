#pragma once
#include <stdint.h>
#include "cJSON.h"
#include "ws2812_driver.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#define LED_NUMBER                  1
#define LED_INDEX_0                 0
#define LED_TIME_REFRESH            1
#define LED_BRIGHTNESS_DEFAULT      50

#define LED_AMBI_PRIMARY_HUE (90)
#define LED_AMBI_PRIMARY_SAT (90)
#define LED_AMBI_SECONDARY_HUE (25)
#define LED_AMBI_SECONDARY_SAT (100)
#define LED_AMBI_ERROR_HUE (2)
#define LED_AMBI_ERROR_SAT (100)

struct RgbColor_t
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

struct HsvColor_t
{
    uint8_t h;
    uint8_t s;
    uint8_t v;
};

class LED
{
    public:
        void Init(void);

        void Set(bool state);

        void Blink(uint32_t changeRateMS);
        
        void FadeIn(void);

        void SetBrightness(uint8_t brightness);

		void SetLedBrightness(const cJSON * const root);

        void SetColor(uint8_t Hue, uint8_t Saturation);

        void Run(void);

    private:
        uint8_t mDefaultOnBrightness;

        uint8_t mHue;       
        uint8_t mSaturation; 
        
		uint32_t mLedLevel;
		
        bool mState;

        void UpdateStableState(void);
};
RgbColor_t HsvToRgb(HsvColor_t hsv);
