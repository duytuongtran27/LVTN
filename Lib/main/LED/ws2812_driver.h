#ifndef __WS2812__
#define __WS2812__

#pragma once
#include "hw_config.h"
#include "driver/gpio.h"
#ifdef __cplusplus
extern "C" {
#endif

esp_err_t ws2812_spi_init(uint8_t channel, uint8_t gpio, uint16_t led_num);

esp_err_t ws2812_spi_deinit(void);

esp_err_t ws2812_spi_set(uint32_t index, uint8_t red, uint8_t green, uint8_t blue);

esp_err_t ws2812_spi_refresh(uint32_t timeout_ms);

esp_err_t ws2812_spi_clear(uint32_t timeout_ms);

esp_err_t ws2812_spi_delete(void);

#ifdef __cplusplus
}
#endif

#endif //__WS2812__