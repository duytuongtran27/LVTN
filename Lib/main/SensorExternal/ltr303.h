#ifndef __LTR303_H__
#define __LTR303_H__
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <i2cdev.h>
#include <esp_err.h>
#include "esp_log.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    als_integration_time_100ms = 0,
    als_integration_time_50ms,
    als_integration_time_200ms,
    als_integration_time_400ms,
    als_integration_time_150ms,
    als_integration_time_250ms,
    als_integration_time_300ms,
    als_integration_time_350ms,
} als_integration_time_t;

typedef enum {
    als_measurement_rate_50ms = 0,
    als_measurement_rate_100ms,
    als_measurement_rate_200ms,
    als_measurement_rate_500ms,
    als_measurement_rate_1000ms,
    als_measurement_rate_2000ms,
} als_measurement_rate_t;

typedef enum {
	als_gain_1x = 0,
	als_gain_2x,
	als_gain_4x,
	als_gain_8x,
	als_gain_48x = 6,
	als_gain_96x,
} als_gain_t;

typedef enum {
    als_mode_standby = 0,
    als_mode_active,
} als_mode_t;

typedef enum {
    als_reset_false = 0,
    als_reset_true,
} als_reset_t;

struct als_raw_data_t
{
    uint16_t als_data_ch0;
    uint16_t als_data_ch1;
};

/**
 * Device descriptor
 */
typedef struct
{
    i2c_dev_t i2c_dev;            //!< I2C device descriptor

    als_integration_time_t als_intergration_time;       
    als_measurement_rate_t als_measurement_rate; 
    als_gain_t als_gain;
    als_mode_t als_mode;
    als_reset_t als_reset;
    struct als_raw_data_t raw_data;
} ltr303_t;

esp_err_t write_register8(i2c_dev_t *dev, uint8_t addr, uint8_t value);

esp_err_t ltr303_init_desc(ltr303_t *dev, i2c_port_t port, gpio_num_t sda_gpio, gpio_num_t scl_gpio);
esp_err_t ltr303_free_desc(ltr303_t *dev);

uint8_t get_gain_cmd(ltr303_t *dev);
uint8_t get_measurement_rate_cmd(ltr303_t *dev);
uint8_t get_integration_time_cmd(ltr303_t *dev);
uint8_t get_mode_cmd(ltr303_t *dev);
uint8_t get_reset_cmd(ltr303_t *dev);

esp_err_t ltr303_set_param(ltr303_t *dev);
esp_err_t ltr303_get_part_id(ltr303_t *dev);
esp_err_t ltr303_get_manufact_id(ltr303_t *dev);
esp_err_t ltr303_get_status(ltr303_t *dev);
esp_err_t ltr303_get_raw_data(ltr303_t *dev);
esp_err_t ltr303_get_lux_data(ltr303_t *dev, float *lux, uint16_t *fspec, uint16_t *inspec);
esp_err_t ltr303_init(ltr303_t *dev);

#ifdef __cplusplus
}
#endif
#endif /* __LTR303_H__ */