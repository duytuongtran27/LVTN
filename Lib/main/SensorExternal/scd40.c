#include "scd40.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>

#define I2C_FREQ_HZ                                 100000 // 100kHz
#define SCD4X_I2C_ADDR                              0x62
#define CMD_START_PERIODIC_MEASUREMENT             (0x21B1)
#define CMD_READ_MEASUREMENT                       (0xEC05)
#define CMD_STOP_PERIODIC_MEASUREMENT              (0x3F86)
#define CMD_SET_TEMPERATURE_OFFSET                 (0x241D)
#define CMD_GET_TEMPERATURE_OFFSET                 (0x2318)
#define CMD_SET_SENSOR_ALTITUDE                    (0x2427)
#define CMD_GET_SENSOR_ALTITUDE                    (0x2322)
#define CMD_SET_AMBIENT_PRESSURE                   (0xE000)
#define CMD_PERFORM_FORCED_RECALIBRATION           (0x362F)
#define CMD_SET_AUTOMATIC_SELF_CALIBRATION_ENABLED (0x2416)
#define CMD_GET_AUTOMATIC_SELF_CALIBRATION_ENABLED (0x2313)
#define CMD_START_LOW_POWER_PERIODIC_MEASUREMENT   (0x21AC)
#define CMD_GET_DATA_READY_STATUS                  (0xE4B8)
#define CMD_PERSIST_SETTINGS                       (0x3615)
#define CMD_GET_SERIAL_NUMBER                      (0x3682)
#define CMD_PERFORM_SELF_TEST                      (0x3639)
#define CMD_PERFORM_FACTORY_RESET                  (0x3632)
#define CMD_REINIT                                 (0x3646)
#define CMD_MEASURE_SINGLE_SHOT                    (0x219D)
#define CMD_MEASURE_SINGLE_SHOT_RHT_ONLY           (0x2196)
#define CMD_POWER_DOWN                             (0x36E0)
#define CMD_WAKE_UP                                (0x36F6)

#define CHECK(x) do { esp_err_t __; if ((__ = x) != ESP_OK) return __; } while (0)
#define CHECK_ARG(VAL) do { if (!(VAL)) return ESP_ERR_INVALID_ARG; } while (0)

static const char *TAG = "scd4x";
static uint8_t crc8(const uint8_t *data, size_t count)
{
    uint8_t res = 0xff;

    for (size_t i = 0; i < count; ++i)
    {
        res ^= data[i];
        for (uint8_t bit = 8; bit > 0; --bit)
        {
            if (res & 0x80)
                res = (res << 1) ^ 0x31;
            else
                res = (res << 1);
        }
    }
    return res;
}

static inline uint16_t swap(uint16_t v)
{
    return (v << 8) | (v >> 8);
}

static esp_err_t send_cmd(scd4x_t *dev, uint16_t cmd, uint16_t *data, size_t words)
{
    uint8_t buf[2 + words * 3];
    // add command
    *(uint16_t *)buf = swap(cmd);
    if (data && words)
        // add arguments
        for (size_t i = 0; i < words; i++)
        {
            uint8_t *p = buf + 2 + i * 3;
            *(uint16_t *)p = swap(data[i]);
            *(p + 2) = crc8(p, 2);
        }

    ESP_LOGV(TAG, "Sending buffer:");
    //ESP_LOG_BUFFER_HEX_LEVEL(TAG, buf, sizeof(buf), ESP_LOG_VERBOSE);

    return i2c_dev_write(&dev->i2c_dev, NULL, 0, buf, sizeof(buf));
}

static esp_err_t read_resp(scd4x_t *dev, uint16_t *data, size_t words)
{
    uint8_t buf[words * 3];
    CHECK(i2c_dev_read(&dev->i2c_dev, NULL, 0, buf, sizeof(buf)));

    ESP_LOGV(TAG, "Received buffer:");
    //ESP_LOG_BUFFER_HEX_LEVEL(TAG, buf, sizeof(buf), ESP_LOG_VERBOSE);

    for (size_t i = 0; i < words; i++)
    {
        uint8_t *p = buf + i * 3;
        uint8_t crc = crc8(p, 2);
        if (crc != *(p + 2))
        {
            ESP_LOGE(TAG, "Invalid CRC 0x%02x, expected 0x%02x", crc, *(p + 2));
            return ESP_ERR_INVALID_CRC;
        }
        data[i] = swap(*(uint16_t *)p);
    }
    return ESP_OK;
}

static esp_err_t execute_cmd(scd4x_t *dev, uint16_t cmd, uint32_t timeout_ms,
        uint16_t *out_data, size_t out_words, uint16_t *in_data, size_t in_words)
{
    CHECK_ARG(dev);

    I2C_DEV_TAKE_MUTEX(&dev->i2c_dev);
    I2C_DEV_CHECK(&dev->i2c_dev, send_cmd(dev, cmd, out_data, out_words));
    if (timeout_ms)
    {
        if (timeout_ms > 10)
            vTaskDelay(pdMS_TO_TICKS(timeout_ms));
        else
            ets_delay_us(timeout_ms * 1000);
    }
    if (in_data && in_words)
    I2C_DEV_CHECK(&dev->i2c_dev, read_resp(dev, in_data, in_words));
    I2C_DEV_GIVE_MUTEX(&dev->i2c_dev);

    return ESP_OK;
}

///////////////////////////////////////////////////////////////////////////////

esp_err_t scd4x_init_desc(scd4x_t *dev, i2c_port_t port, gpio_num_t sda_gpio, gpio_num_t scl_gpio)
{
    CHECK_ARG(dev);

    dev->i2c_dev.port = port;
    dev->i2c_dev.addr = SCD4X_I2C_ADDR;
    dev->i2c_dev.cfg.sda_io_num = sda_gpio;
    dev->i2c_dev.cfg.scl_io_num = scl_gpio;
    dev->i2c_dev.cfg.master.clk_speed = I2C_FREQ_HZ;

    return i2c_dev_create_mutex(&dev->i2c_dev);
}

esp_err_t scd4x_free_desc(scd4x_t *dev)
{
    CHECK_ARG(dev);

    return i2c_dev_delete_mutex(&dev->i2c_dev);
}

esp_err_t scd4x_start_periodic_measurement(scd4x_t *dev)
{
    return execute_cmd(dev, CMD_START_PERIODIC_MEASUREMENT, 5000, NULL, 0, NULL, 0);
}

esp_err_t scd4x_read_measurement_ticks(scd4x_t *dev, uint16_t *co2, uint16_t *temperature, uint16_t *humidity)
{
    CHECK_ARG(co2 || temperature || humidity);

    uint16_t buf[3];
    CHECK(execute_cmd(dev, CMD_READ_MEASUREMENT, 2, NULL, 0, buf, 3));
    if (co2)
        *co2 = buf[0];
    if (temperature)
        *temperature = buf[1];
    if (humidity)
        *humidity = buf[2];

    return ESP_OK;
}

esp_err_t scd4x_read_measurement(scd4x_t *dev, uint16_t *co2, float *temperature, float *humidity)
{
    CHECK_ARG(co2 || temperature || humidity);
    uint16_t t_raw, h_raw;

    CHECK(scd4x_read_measurement_ticks(dev, co2, &t_raw, &h_raw));
    if (temperature)
        *temperature = (float)t_raw * 175.0f / 65536.0f - 45.0f;
    if (humidity)
        *humidity = (float)h_raw * 100.0f / 65536.0f;

    return ESP_OK;
}

esp_err_t scd4x_stop_periodic_measurement(scd4x_t *dev)
{
    return execute_cmd(dev, CMD_STOP_PERIODIC_MEASUREMENT, 500, NULL, 0, NULL, 0);
}

esp_err_t scd4x_get_temperature_offset_ticks(scd4x_t *dev, uint16_t *t_offset)
{
    CHECK_ARG(t_offset);

    return execute_cmd(dev, CMD_GET_TEMPERATURE_OFFSET, 1, NULL, 0, t_offset, 1);
}

esp_err_t scd4x_get_temperature_offset(scd4x_t *dev, float *t_offset)
{
    CHECK_ARG(t_offset);
    uint16_t raw;

    CHECK(scd4x_get_temperature_offset_ticks(dev, &raw));

    *t_offset = (float)raw * 175.0f / 65536.0f;

    return ESP_OK;
}

esp_err_t scd4x_set_temperature_offset_ticks(scd4x_t *dev, uint16_t t_offset)
{
    return execute_cmd(dev, CMD_SET_TEMPERATURE_OFFSET, 1, &t_offset, 1, NULL, 0);
}

esp_err_t scd4x_set_temperature_offset(scd4x_t *dev, float t_offset)
{
    uint16_t raw = (uint16_t)(t_offset * 65536.0f / 175.0f + 0.5f);
    return scd4x_set_temperature_offset_ticks(dev, raw);
}

esp_err_t scd4x_get_sensor_altitude(scd4x_t *dev, uint16_t *altitude)
{
    CHECK_ARG(altitude);

    return execute_cmd(dev, CMD_GET_SENSOR_ALTITUDE, 1, NULL, 0, altitude, 1);
}

esp_err_t scd4x_set_sensor_altitude(scd4x_t *dev, uint16_t altitude)
{
    return execute_cmd(dev, CMD_SET_SENSOR_ALTITUDE, 1, &altitude, 1, NULL, 0);
}

esp_err_t scd4x_set_ambient_ressure(scd4x_t *dev, uint16_t pressure)
{
    return execute_cmd(dev, CMD_SET_AMBIENT_PRESSURE, 1, &pressure, 1, NULL, 0);
}

esp_err_t scd4x_perform_forced_recalibration(scd4x_t *dev, uint16_t target_co2_concentration,
        uint16_t *frc_correction)
{
    CHECK_ARG(frc_correction);

    return execute_cmd(dev, CMD_PERFORM_FORCED_RECALIBRATION, 400,
            &target_co2_concentration, 1, frc_correction, 1);
}

esp_err_t scd4x_get_automatic_self_calibration(scd4x_t *dev, bool *enabled)
{
    CHECK_ARG(enabled);

    return execute_cmd(dev, CMD_GET_AUTOMATIC_SELF_CALIBRATION_ENABLED, 1, NULL, 0, (uint16_t *)enabled, 1);
}

esp_err_t scd4x_set_automatic_self_calibration(scd4x_t *dev, bool enabled)
{
    return execute_cmd(dev, CMD_SET_AUTOMATIC_SELF_CALIBRATION_ENABLED, 1, (uint16_t *)&enabled, 1, NULL, 0);
}

esp_err_t scd4x_start_low_power_periodic_measurement(scd4x_t *dev)
{
    return execute_cmd(dev, CMD_START_LOW_POWER_PERIODIC_MEASUREMENT, 0, NULL, 0, NULL, 0);
}

esp_err_t scd4x_get_data_ready_status(scd4x_t *dev, bool *data_ready)
{
    CHECK_ARG(data_ready);

    uint16_t status;
    CHECK(execute_cmd(dev, CMD_GET_DATA_READY_STATUS, 1, NULL, 0, &status, 1));
    *data_ready = (status & 0x7ff) != 0;

    return ESP_OK;
}

esp_err_t scd4x_persist_settings(scd4x_t *dev)
{
    return execute_cmd(dev, CMD_PERSIST_SETTINGS, 800, NULL, 0, NULL, 0);
}

esp_err_t scd4x_get_serial_number(scd4x_t *dev, uint16_t *serial0, uint16_t *serial1, uint16_t *serial2)
{
    CHECK_ARG(serial0 && serial1 && serial2);

    uint16_t buf[3];
    CHECK(execute_cmd(dev, CMD_GET_SERIAL_NUMBER, 1, NULL, 0, buf, 3));
    *serial0 = buf[0];
    *serial1 = buf[1];
    *serial2 = buf[2];

    return ESP_OK;
}

esp_err_t scd4x_perform_self_test(scd4x_t *dev, bool *malfunction)
{
    CHECK_ARG(malfunction);

    return execute_cmd(dev, CMD_PERFORM_SELF_TEST, 10000, NULL, 0, (uint16_t *)malfunction, 1);
}

esp_err_t scd4x_perform_factory_reset(scd4x_t *dev)
{
    return execute_cmd(dev, CMD_PERFORM_FACTORY_RESET, 800, NULL, 0, NULL, 0);
}

esp_err_t scd4x_reinit(scd4x_t *dev)
{
    return execute_cmd(dev, CMD_REINIT, 30, NULL, 0, NULL, 0);
}

esp_err_t scd4x_measure_single_shot(scd4x_t *dev)
{
    return execute_cmd(dev, CMD_MEASURE_SINGLE_SHOT, 5000, NULL, 0, NULL, 0);
}

esp_err_t scd4x_measure_single_shot_rht_only(scd4x_t *dev)
{
    return execute_cmd(dev, CMD_MEASURE_SINGLE_SHOT_RHT_ONLY, 50, NULL, 0, NULL, 0);
}

esp_err_t scd4x_power_down(scd4x_t *dev)
{
    return execute_cmd(dev, CMD_POWER_DOWN, 1, NULL, 0, NULL, 0);
}

esp_err_t scd4x_wake_up(scd4x_t *dev)
{
    return execute_cmd(dev, CMD_WAKE_UP, 20, NULL, 0, NULL, 0);
}