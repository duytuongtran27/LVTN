#include "at24c.h"

#define I2C_FREQ_HZ 				1000000 // 1MHz

#define AT24_DEV_ADDR     			(0x50)

#define CHECK(x) 					do { esp_err_t __; if ((__ = x) != ESP_OK) return __; } while (0)
#define CHECK_ARG(VAL) 				do { if (!(VAL)) return ESP_ERR_INVALID_ARG; } while (0)

static const char *TAG = "AT24CXX";

uint16_t MaxAddress(at24c_t * dev) {
	return dev->_bytes;
}

esp_err_t at24cx_init_desc(at24c_t *dev, i2c_port_t port, gpio_num_t sda_gpio, gpio_num_t scl_gpio)
{
    CHECK_ARG(dev);

    //at24c128
    // int16_t size = EEPROM_TYPE;

    dev->_size = dev->size;
	dev->_bytes = 128 * dev->size;

    dev->i2c_dev.port = port;
    dev->i2c_dev.addr = AT24_DEV_ADDR;
    dev->i2c_dev.cfg.sda_io_num = sda_gpio;
    dev->i2c_dev.cfg.scl_io_num = scl_gpio;
    dev->i2c_dev.cfg.master.clk_speed = I2C_FREQ_HZ;

    return i2c_dev_create_mutex(&dev->i2c_dev);
}

esp_err_t at24cx_free_desc(at24c_t *dev)
{
    CHECK_ARG(dev);

    return i2c_dev_delete_mutex(&dev->i2c_dev);
}

esp_err_t WriteRom(at24c_t * dev, uint16_t data_addr, uint8_t data)
{
    CHECK_ARG(dev);
	I2C_DEV_TAKE_MUTEX(&dev->i2c_dev);

	if (data_addr > dev->_bytes) return 0;

	if (dev->_size < 32) {
		int blockNumber = data_addr / 256;
		uint16_t _data_addr = data_addr - (blockNumber * 256);
		int _chip_addr = dev->i2c_dev.addr + blockNumber;
		ESP_LOGD(TAG, "WriteRom dev_addr =%x _data_addr=%d", dev->i2c_dev.addr, _data_addr);
		//return WriteReg8(dev, dev->i2c_dev.port, _chip_addr, _data_addr, data);
		i2c_dev_write_8(&dev->i2c_dev,_chip_addr,_data_addr, data);
	} else {
		ESP_LOGD(TAG, "WriteRom dev_addr =%x _data_addr=%d", dev->i2c_dev.addr, data_addr);
		CHECK(i2c_dev_write_16(&dev->i2c_dev, data_addr, data));
	}
    I2C_DEV_GIVE_MUTEX(&dev->i2c_dev);
    return ESP_OK;
}
esp_err_t ReadRom(at24c_t * dev, uint16_t data_addr, uint8_t * data)
{
    CHECK_ARG(dev);
    I2C_DEV_TAKE_MUTEX(&dev->i2c_dev);

	if (data_addr > dev->_bytes) return 0;

	if (dev->_size < 32) {
		int blockNumber = data_addr / 256;
		uint16_t _data_addr = data_addr - (blockNumber * 256);
		int _chip_addr = dev->i2c_dev.addr + blockNumber;
		ESP_LOGD(TAG, "ReadRom dev_addr=%x data_addr=%d", dev->i2c_dev.addr, _data_addr);
		//return WriteReg8(dev, dev->i2c_dev.port, _chip_addr, _data_addr, data);
		i2c_dev_read_8(&dev->i2c_dev, _chip_addr,_data_addr, data);
	} else {
		ESP_LOGD(TAG, "ReadRom dev_addr=%x data_addr=%d", dev->i2c_dev.addr, data_addr);
		CHECK(i2c_dev_read_16(&dev->i2c_dev, data_addr, data));
	}
    I2C_DEV_GIVE_MUTEX(&dev->i2c_dev);
    return ESP_OK;
}