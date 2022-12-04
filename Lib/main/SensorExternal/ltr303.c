#include "ltr303.h"

#define I2C_FREQ_HZ 						100000 // 100kHz //1000000 // 1MHz
#define LTR303_DEV_ADDR     				(0x29)
//ALS REGISTER
#define ALS_CONTR           				(0x80)
#define ALS_MEAS_RATE       				(0x85)
#define PART_ID             				(0x86)
#define MANUFACT_ID         				(0x87)
#define ALS_DATA_CH1_0      				(0x88)
#define ALS_DATA_CH1_1      				(0x89)
#define ALS_DATA_CH0_0      				(0x8a)
#define ALS_DATA_CH0_1      				(0x8b)
#define ALS_STATUS          				(0x8c)
#define INTERRUPT_REG       				(0x8f)
#define ALS_THRES_UP_0      				(0x97)
#define ALS_THRES_UP_1      				(0x98)
#define ALS_THRES_LOW_0     				(0x99)
#define ALS_THRES_LOW_1     				(0x9a)
#define INTERRUPT_PERSIST   				(0x9e)
//ALS COMMAND		
#define CMD_ALS_MODE_STANDBY 				(0X00)
#define CMD_ALS_MODE_ACTIVE					(0X01)

#define CMD_ALS_RESET_FALSE 				(0X00)
#define CMD_ALS_RESET_TRUE					(0X01)

#define CMD_ALS_GAIN_1X						(0X00)
#define CMD_ALS_GAIN_2X						(0X01)
#define CMD_ALS_GAIN_4X						(0X02)
#define CMD_ALS_GAIN_8X						(0X03)
#define CMD_ALS_GAIN_48X					(0X06)
#define CMD_ALS_GAIN_96X					(0X07)

#define CMD_ALS_MEASUREMENT_RATE_50MS		(0X00)
#define CMD_ALS_MEASUREMENT_RATE_100MS		(0X01)
#define CMD_ALS_MEASUREMENT_RATE_200MS		(0X02)
#define CMD_ALS_MEASUREMENT_RATE_500MS		(0X03)
#define CMD_ALS_MEASUREMENT_RATE_1000MS		(0X04)
#define CMD_ALS_MEASUREMENT_RATE_2000MS		(0X05)

#define CMD_ALS_INTEGRATION_TIME_100MS  	(0X00)
#define CMD_ALS_INTEGRATION_TIME_50MS		(0X01)
#define CMD_ALS_INTEGRATION_TIME_200MS  	(0X02)
#define CMD_ALS_INTEGRATION_TIME_400MS  	(0X03)
#define CMD_ALS_INTEGRATION_TIME_150MS  	(0X04)
#define CMD_ALS_INTEGRATION_TIME_250MS  	(0X05)
#define CMD_ALS_INTEGRATION_TIME_300MS  	(0X06)
#define CMD_ALS_INTEGRATION_TIME_350MS  	(0X07)

#define CHECK(x) do { esp_err_t __; if ((__ = x) != ESP_OK) return __; } while (0)
#define CHECK_ARG(VAL) do { if (!(VAL)) return ESP_ERR_INVALID_ARG; } while (0)
#define CHECK_LOGE(dev, x, msg, ...) do { \
        esp_err_t __; \
        if ((__ = x) != ESP_OK) { \
            I2C_DEV_GIVE_MUTEX(&dev->i2c_dev); \
            ESP_LOGE(TAG, msg, ## __VA_ARGS__); \
            return __; \
        } \
    } while (0)

static const char *TAG = "LTR303 SENSOR";

esp_err_t write_register8(i2c_dev_t *dev, uint8_t addr, uint8_t value)
{
    return i2c_dev_write_reg(dev, addr, &value, 1);
}

esp_err_t ltr303_init_desc(ltr303_t *dev, i2c_port_t port, gpio_num_t sda_gpio, gpio_num_t scl_gpio)
{
    CHECK_ARG(dev);

    dev->i2c_dev.port = port;
    dev->i2c_dev.addr = LTR303_DEV_ADDR;
    dev->i2c_dev.cfg.sda_io_num = sda_gpio;
    dev->i2c_dev.cfg.scl_io_num = scl_gpio;

    dev->i2c_dev.cfg.master.clk_speed = I2C_FREQ_HZ;

    return i2c_dev_create_mutex(&dev->i2c_dev);
}
esp_err_t ltr303_free_desc(ltr303_t *dev)
{
    CHECK_ARG(dev);

    return i2c_dev_delete_mutex(&dev->i2c_dev);
}
uint8_t get_gain_cmd(ltr303_t *dev)
{
    switch (dev->als_gain)
    {
        case als_gain_2x:
            return CMD_ALS_GAIN_2X;
        case als_gain_4x:
            return CMD_ALS_GAIN_4X;
        case als_gain_8x:
            return CMD_ALS_GAIN_8X;
        case als_gain_48x:
            return CMD_ALS_GAIN_48X;
        case als_gain_96x:
            return CMD_ALS_GAIN_96X;
        default:
		    return CMD_ALS_GAIN_1X;
    }
}
uint8_t get_measurement_rate_cmd(ltr303_t *dev)
{
    switch (dev->als_measurement_rate)
    {
        case als_measurement_rate_100ms:
            return CMD_ALS_MEASUREMENT_RATE_100MS;
        case als_measurement_rate_200ms:
            return CMD_ALS_MEASUREMENT_RATE_200MS;
        case als_measurement_rate_500ms:
            return CMD_ALS_MEASUREMENT_RATE_500MS;
        case als_measurement_rate_1000ms:
            return CMD_ALS_MEASUREMENT_RATE_1000MS;
		case als_measurement_rate_2000ms:
            return CMD_ALS_MEASUREMENT_RATE_2000MS;
        default:
		    return CMD_ALS_MEASUREMENT_RATE_50MS;
    }
}
uint8_t get_integration_time_cmd(ltr303_t *dev)
{
    switch (dev->als_intergration_time)
    {
        case als_integration_time_50ms:
            return CMD_ALS_INTEGRATION_TIME_50MS;
        case als_integration_time_200ms:
            return CMD_ALS_INTEGRATION_TIME_200MS;
        case als_integration_time_400ms:
            return CMD_ALS_INTEGRATION_TIME_400MS;
        case als_integration_time_150ms:
            return CMD_ALS_INTEGRATION_TIME_150MS;
		case als_integration_time_250ms:
            return CMD_ALS_INTEGRATION_TIME_250MS;
		case als_integration_time_300ms:
            return CMD_ALS_INTEGRATION_TIME_300MS;
		case als_integration_time_350ms:
            return CMD_ALS_INTEGRATION_TIME_350MS;
        default:
		    return CMD_ALS_INTEGRATION_TIME_100MS;
    }
}
uint8_t get_mode_cmd(ltr303_t *dev){
	switch (dev->als_mode)
    {
		case als_mode_active:
            return CMD_ALS_MODE_ACTIVE;
        default:
		    return CMD_ALS_MODE_STANDBY;
    }
}
uint8_t get_reset_cmd(ltr303_t *dev){
	switch (dev->als_reset)
    {
		case als_reset_true:
            return CMD_ALS_RESET_TRUE;
        default:
		    return CMD_ALS_RESET_FALSE;
    }
}
esp_err_t ltr303_set_param(ltr303_t *dev)
{
	dev->als_gain = als_gain_1x;
	dev->als_intergration_time = als_integration_time_200ms;
	dev->als_measurement_rate = als_measurement_rate_200ms;
	dev->als_mode = als_mode_active;
	dev->als_reset = als_reset_false;
	return ESP_OK;
}
esp_err_t ltr303_get_part_id(ltr303_t *dev)
{
	CHECK_ARG(dev);
	I2C_DEV_TAKE_MUTEX(&dev->i2c_dev);
	
	uint8_t part_id=0x00;

	CHECK(i2c_dev_read_reg(&dev->i2c_dev, PART_ID, &part_id, 1));
	ESP_LOGE(TAG, "PART ID=%d", part_id);

	I2C_DEV_GIVE_MUTEX(&dev->i2c_dev);
    return ESP_OK;
}
esp_err_t ltr303_get_manufact_id(ltr303_t *dev)
{
	CHECK_ARG(dev);
	I2C_DEV_TAKE_MUTEX(&dev->i2c_dev);
	
	uint8_t manufact_id=0x00;

	CHECK(i2c_dev_read_reg(&dev->i2c_dev, MANUFACT_ID, &manufact_id, 1));
	ESP_LOGE(TAG, "MANUFACT ID=%d", manufact_id);
	
	I2C_DEV_GIVE_MUTEX(&dev->i2c_dev);
    return ESP_OK;
}
esp_err_t ltr303_get_status(ltr303_t *dev)
{
	CHECK_ARG(dev);
	I2C_DEV_TAKE_MUTEX(&dev->i2c_dev);

	uint8_t status=0x00,interrupt_status=0x00,newdata_status=0x00,als_data_valid=0x00;
	CHECK(i2c_dev_read_reg(&dev->i2c_dev, ALS_STATUS, &status, 1));

	interrupt_status = status & 0x08 ;
	newdata_status = status & 0x04 ;
	als_data_valid = status & 0x80;

	ESP_LOGE(TAG, "status=%d", status);
	ESP_LOGE(TAG, "interrupt status=%d", interrupt_status);
    ESP_LOGE(TAG, "newdata status=%d", newdata_status);
    ESP_LOGE(TAG, "als_data valid=%d", als_data_valid);
	
	I2C_DEV_GIVE_MUTEX(&dev->i2c_dev);
    return ESP_OK;
}
esp_err_t ltr303_get_lux_data(ltr303_t *dev, float *lux, uint16_t *fspec, uint16_t *inspec)
{	
	float ratio;
	uint16_t ch0,ch1;

	ch0=dev->raw_data.als_data_ch0;
	ch1=dev->raw_data.als_data_ch1;

	*fspec=ch0;
	*inspec=ch1;

	// ESP_LOGE(TAG, "raw_data.als_data_ch0=%d", ch0);
	// ESP_LOGE(TAG, "raw_data.als_data_ch1=%d", ch1);
	if ((ch0 == 0xFFFF) || (ch1 == 0xFFFF)) {
		*lux = 0.0;
	}
	// We will need the ratio for subsequent calculations
	ratio = ch1 /(ch0+ch1);

	if (ratio < 0.45)
    {
        *lux = ( (1.7743*ch0) + (1.1059*ch1) ) /1/2;
		//ESP_LOGE(TAG, "Lux=%f", lux);
		return ESP_OK;
    }
    else if (ratio < 0.64 && ratio >= 0.45)
    {
        *lux = ( (4.2785*ch0) - (1.9548*ch1) ) /1/2;
		//ESP_LOGE(TAG, "Lux=%f", lux);
		return ESP_OK;
    }
    else if (ratio < 0.85 && ratio >= 0.64)
    {
        *lux = ( (0.5926*ch0) + (0.1185*ch1) )  /1/2;
		//ESP_LOGE(TAG, "Lux=%f", lux);
		return ESP_OK;
    }
    else
    {
        *lux = 0;
		//ESP_LOGE(TAG, "Lux=%f", lux);
		return ESP_OK;
    }
	return ESP_OK;
}

esp_err_t ltr303_get_raw_data(ltr303_t *dev)
{
	CHECK_ARG(dev);
	I2C_DEV_TAKE_MUTEX(&dev->i2c_dev);
	uint8_t data0,data1,data2,data3;

	uint16_t ALS_CH1_ADC_Data,ALS_CH0_ADC_Data;

	CHECK(i2c_dev_read_reg(&dev->i2c_dev, ALS_DATA_CH1_0, &data0, 1));
	CHECK(i2c_dev_read_reg(&dev->i2c_dev, ALS_DATA_CH1_1, &data1, 1));
	CHECK(i2c_dev_read_reg(&dev->i2c_dev, ALS_DATA_CH0_0, &data2, 1));
	CHECK(i2c_dev_read_reg(&dev->i2c_dev, ALS_DATA_CH0_1, &data3, 1));

	ALS_CH1_ADC_Data = (data1 << 8) | data0 ;// Combining lower and upper bytes to give 16-bit Ch1 data
	ALS_CH0_ADC_Data = (data3 << 8) | data2 ;// Combining lower and upper bytes to give 16-bit Ch0 data

	// ESP_LOGE(TAG, "ALS_DATA_CH1_0=%d", data0);
	// ESP_LOGE(TAG, "ALS_DATA_CH1_1=%d", data1);
	// ESP_LOGE(TAG, "ALS_DATA_CH0_0=%d", data2);
	// ESP_LOGE(TAG, "ALS_DATA_CH0_1=%d", data3);

	// ESP_LOGE(TAG, "ALS_CH1_ADC_Data=%d", ALS_CH1_ADC_Data);
	// ESP_LOGE(TAG, "ALS_CH0_ADC_Data=%d", ALS_CH0_ADC_Data);

	dev->raw_data.als_data_ch0=ALS_CH0_ADC_Data;
	dev->raw_data.als_data_ch1=ALS_CH1_ADC_Data;

	I2C_DEV_GIVE_MUTEX(&dev->i2c_dev);
	return ESP_OK;
}
esp_err_t ltr303_init(ltr303_t *dev)
{
    CHECK_ARG(dev);
	I2C_DEV_TAKE_MUTEX(&dev->i2c_dev);

	ltr303_set_param(dev);

	uint8_t gain = get_gain_cmd(dev);
    uint8_t reset = get_reset_cmd(dev);
    uint8_t mode = get_mode_cmd(dev);
	uint8_t integrationTime = get_integration_time_cmd(dev);
	uint8_t measurementRate = get_measurement_rate_cmd(dev);

	uint8_t control = 0x00;
	uint8_t measurement = 0x00;

	// Perform sanity checks
	if (gain > 0x03 && gain < 0x06) 
	{
		gain = 0x00;
	}else if(gain > 0x07) 
	{
		gain = 0x00;
	}

	// control byte logic
	control |= gain << 2;
	if(reset) 
	{
		control |= 0x02;
	}

	if(mode) 
	{
		control |= 0x01;
	}

	CHECK_LOGE(dev, write_register8(&dev->i2c_dev,ALS_CONTR,control), "Failed to set control register");

	// Perform sanity checks
	if(integrationTime > 0x07) {
		integrationTime = 0x00;
	}
	
	if(measurementRate > 0x05) {
		measurementRate = 0x00;
	}
	
	measurement |= integrationTime << 3;
	measurement |= measurementRate;

	CHECK_LOGE(dev,write_register8(&dev->i2c_dev,ALS_MEAS_RATE,measurement), "Failed to set mesurement register");

	I2C_DEV_GIVE_MUTEX(&dev->i2c_dev);

    return ESP_OK;	
}