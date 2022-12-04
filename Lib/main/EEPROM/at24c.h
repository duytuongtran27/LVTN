#pragma once

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

typedef struct
{
    i2c_dev_t i2c_dev;
    uint16_t size;        
    uint16_t _size;
    uint16_t _bytes;
} at24c_t;

uint16_t MaxAddress(at24c_t *dev);
esp_err_t at24cx_init_desc(at24c_t *dev, i2c_port_t port, gpio_num_t sda_gpio, gpio_num_t scl_gpio);
esp_err_t at24cx_free_desc(at24c_t *dev);
esp_err_t ReadRom(at24c_t *dev,uint16_t data_addr, uint8_t *data);
esp_err_t WriteRom(at24c_t *dev, uint16_t data_addr, uint8_t data);

#ifdef __cplusplus
}
#endif