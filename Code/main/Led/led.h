#ifndef _LED_H_
#define _LED_H_

#include "esp_err.h"

    esp_err_t Led_init();
    esp_err_t set_led_level(uint32_t level);
    esp_err_t reset_led_pin();

#endif