#ifndef __SGM62110_H__
#define __SGM62110_H__

#include <i2cdev.h>
#include <esp_err.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    sgm_slew_rate_1V= 0,
    sgm_slew_rate_2_5V,
    sgm_slew_rate_5V,
    sgm_slew_rate_10V,
} sgm_slew_rate_t;

typedef enum {
    sgm_rpwm_dis= 0,
    sgm_rpwm_en,
} sgm_rpwm_t;

typedef enum {
    sgm_fpwm_dis= 0,
    sgm_fpwm_en,
} sgm_fpwm_t;

typedef enum {
    sgm_conv_dis= 0,
    sgm_conv_en,
} sgm_conv_t;

typedef enum {
    sgm_low_range= 0,
    sgm_high_range,
} sgm_range_t;

/**
 * Device descriptor
 */
typedef struct
{
    i2c_dev_t i2c_dev;            //!< I2C device descriptor
    sgm_slew_rate_t sgm_slew_rate;
    sgm_rpwm_t sgm_rpwm;
    sgm_fpwm_t sgm_fpwm;
    sgm_conv_t sgm_conv;
    sgm_range_t sgm_range;
} sgm62110_t;

esp_err_t write_reg8(i2c_dev_t *dev, uint8_t addr, uint8_t value);

esp_err_t sgm62110_init_desc(sgm62110_t *dev, i2c_port_t port, gpio_num_t sda_gpio, gpio_num_t scl_gpio);
esp_err_t sgm62110_free_desc(sgm62110_t *dev);

uint8_t get_slew_rate_cmd(sgm62110_t *dev);
bool get_rpwm_cmd(sgm62110_t *dev);
bool get_fpwm_cmd(sgm62110_t *dev);
bool get_conv_cmd(sgm62110_t *dev);
bool get_range_cmd(sgm62110_t *dev);

esp_err_t sgm62110_set_param(sgm62110_t *dev);
esp_err_t sgm62110_get_param(sgm62110_t *dev);
esp_err_t sgm62110_get_dev_id(sgm62110_t *dev);
esp_err_t sgm62110_get_status(sgm62110_t *dev);
esp_err_t sgm62110_init(sgm62110_t *dev);
esp_err_t sgm62110_set_vout1(sgm62110_t *dev, uint8_t vset1);
esp_err_t sgm62110_set_vout2(sgm62110_t *dev, uint8_t vset2);

#ifdef __cplusplus
}
#endif

#endif /* __SGM62110_H__ */