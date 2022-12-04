#include "sgm62110.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>

#define I2C_FREQ_HZ                                 (100000) // 100kHz
#define SGM62110_I2C_ADDR                           (0x75)
//SGM REGISTER
#define SGM_CONTROL           				        (0x01)
#define SGM_STATUS           				        (0x02)
#define SGM_DEVID           				        (0x03)
#define SGM_VOUT1           				        (0x04)
#define SGM_VOUT2           				        (0x05)
//VOUT1 REGISTER
#define SGM_DEFAULT_VOUT1_R0                        (3300)
#define SGM_MIN_VOUT1_R0                            (1800)
#define SGM_MAX_VOUT1_R0                            (4975)

#define SGM_DEFAULT_VOUT1_R1                        (3525)
#define SGM_MIN_VOUT1_R1                            (2025)
#define SGM_MAX_VOUT1_R1                            (5200)

#define SGM_VOUT1_STEP                              (25)
//VOUT2 REGISTER
#define SGM_DEFAULT_VOUT2_R0                        (3450)
#define SGM_MIN_VOUT2_R0                            (1800)
#define SGM_MAX_VOUT2_R0                            (4975)

#define SGM_DEFAULT_VOUT2_R1                        (3675)
#define SGM_MIN_VOUT2_R1                            (2025)
#define SGM_MAX_VOUT2_R1                            (5200)

#define SGM_VOUT2_STEP                              (25)
//SGM COMMAND
#define CMD_SGM_SLEW_RATE_1V           	            (0x00)
#define CMD_SGM_SLEW_RATE_2_5V           	        (0x01)
#define CMD_SGM_SLEW_RATE_5V           	            (0x02)
#define CMD_SGM_SLEW_RATE_10V           	        (0x03)

#define CMD_SGM_RPWM_DIS                            (0x00)
#define CMD_SGM_RPWM_EN                             (0x01)

#define CMD_SGM_FPWM_DIS                            (0x00)
#define CMD_SGM_FPWM_EN                             (0x01)

#define CMD_SGM_CONV_DIS                            (0x00)
#define CMD_SGM_CONV_EN                             (0x01)

#define CMD_SGM_LOW_RANGE                           (0x00)
#define CMD_SGM_HIGH_RANGE                          (0x01)

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

static const char *TAG = "SGM62110";

esp_err_t write_reg8(i2c_dev_t *dev, uint8_t addr, uint8_t value)
{
    return i2c_dev_write_reg(dev, addr, &value, 1);
}

esp_err_t sgm62110_init_desc(sgm62110_t *dev, i2c_port_t port, gpio_num_t sda_gpio, gpio_num_t scl_gpio)
{
    CHECK_ARG(dev);

    dev->i2c_dev.port = port;
    dev->i2c_dev.addr = SGM62110_I2C_ADDR;
    dev->i2c_dev.cfg.sda_io_num = sda_gpio;
    dev->i2c_dev.cfg.scl_io_num = scl_gpio;

    dev->i2c_dev.cfg.master.clk_speed = I2C_FREQ_HZ;

    return i2c_dev_create_mutex(&dev->i2c_dev);
}
esp_err_t sgm62110_free_desc(sgm62110_t *dev)
{
    CHECK_ARG(dev);

    return i2c_dev_delete_mutex(&dev->i2c_dev);
}
uint8_t get_slew_rate_cmd(sgm62110_t *dev)
{
    switch (dev->sgm_slew_rate)
    {
        case sgm_slew_rate_2_5V:
            return CMD_SGM_SLEW_RATE_2_5V;
        case sgm_slew_rate_5V:
            return CMD_SGM_SLEW_RATE_5V;
        case sgm_slew_rate_10V:
            return CMD_SGM_SLEW_RATE_10V;
        default:
            return CMD_SGM_SLEW_RATE_1V;
    }
}
bool get_rpwm_cmd(sgm62110_t *dev)
{
    switch (dev->sgm_rpwm)
    {
        case sgm_rpwm_en:
            return CMD_SGM_RPWM_EN;
        default:
            return CMD_SGM_RPWM_DIS;
    }
}
bool get_fpwm_cmd(sgm62110_t *dev)
{
    switch (dev->sgm_fpwm)
    {
        case sgm_fpwm_en:
            return CMD_SGM_FPWM_EN;
        default:
            return CMD_SGM_FPWM_DIS;
    }
}
bool get_conv_cmd(sgm62110_t *dev)
{
    switch (dev->sgm_conv)
    {
        case sgm_conv_en:
            return CMD_SGM_CONV_EN;
        default:
            return CMD_SGM_CONV_DIS;
    }
}
bool get_range_cmd(sgm62110_t *dev)
{
    switch (dev->sgm_range)
    {
        case sgm_high_range:
            return CMD_SGM_HIGH_RANGE;
        default:
            return CMD_SGM_LOW_RANGE;
    }
}
esp_err_t sgm62110_set_param(sgm62110_t *dev)
{
    dev->sgm_slew_rate=sgm_slew_rate_1V;
    dev->sgm_rpwm=sgm_rpwm_dis;
    dev->sgm_fpwm=sgm_fpwm_dis;
    dev->sgm_conv=sgm_conv_en;
    dev->sgm_range=sgm_high_range;
    return ESP_OK;
}
esp_err_t sgm62110_get_param(sgm62110_t *dev)
{
	CHECK_ARG(dev);
	I2C_DEV_TAKE_MUTEX(&dev->i2c_dev);
	
	uint8_t param=0x00, SL=0x00, RPWM=0x00, FPWM=0x00, CONV=0x00, RANGE=0x00;

	CHECK(i2c_dev_read_reg(&dev->i2c_dev, SGM_CONTROL, &param, 1));

    SL = param & 0x03;
	RPWM = param & 0x04;
	FPWM = param & 0x08;
    CONV = param & 0x20;
    RANGE = param & 0x40;

    ESP_LOGI(TAG, "SGM param = %d", param);
	ESP_LOGI(TAG, "SGM Slew rate = %d", SL);
    ESP_LOGI(TAG, "SGM RPWM = %d", RPWM);
    ESP_LOGI(TAG, "SGM FPWM = %d", FPWM);
    ESP_LOGI(TAG, "SGM CONV = %d", CONV);
    ESP_LOGI(TAG, "SGM RANGE = %d", RANGE);

	I2C_DEV_GIVE_MUTEX(&dev->i2c_dev);
    return ESP_OK;
}
esp_err_t sgm62110_get_dev_id(sgm62110_t *dev)
{
	CHECK_ARG(dev);
	I2C_DEV_TAKE_MUTEX(&dev->i2c_dev);
	
	uint8_t dev_id=0x00;

	CHECK(i2c_dev_read_reg(&dev->i2c_dev, SGM_DEVID, &dev_id, 1));
	ESP_LOGI(TAG, "SGM Identify = %d", dev_id);

	I2C_DEV_GIVE_MUTEX(&dev->i2c_dev);
    return ESP_OK;
}
esp_err_t sgm62110_get_status(sgm62110_t *dev)
{
	CHECK_ARG(dev);
	I2C_DEV_TAKE_MUTEX(&dev->i2c_dev);

	uint8_t status=0x00,PG=0x00,TS=0x00,OC=0x00,HD=0x00;
	CHECK(i2c_dev_read_reg(&dev->i2c_dev, SGM_STATUS, &status, 1));

	PG = status & 0x01;
	TS = status & 0x02;
	OC = status & 0x04;
    HD = status & 0x08;

	ESP_LOGI(TAG, "Status Register = %d", status);
	ESP_LOGI(TAG, "Power Good status = %d", PG);
    ESP_LOGI(TAG, "Thermal Shutdown status = %d", TS);
    ESP_LOGI(TAG, "Over Current status = %d", OC);
    ESP_LOGI(TAG, "Hot Die status = %d", HD);
	
	I2C_DEV_GIVE_MUTEX(&dev->i2c_dev);
    return ESP_OK;
}
esp_err_t sgm62110_init(sgm62110_t *dev)
{
    CHECK_ARG(dev);
	I2C_DEV_TAKE_MUTEX(&dev->i2c_dev);

	sgm62110_set_param(dev);

	uint8_t slew_rate = get_slew_rate_cmd(dev);
    bool rpwm = get_rpwm_cmd(dev);
    bool fpwm = get_fpwm_cmd(dev);
	bool conv = get_conv_cmd(dev);
	bool range = get_range_cmd(dev);

	uint8_t control = 0x00;

	if (slew_rate > 0x03)
	{
		slew_rate = 0x00;
	}
	// control byte logic
	control |= slew_rate;
	if(rpwm) 
	{
		control |= 0x04;
	}

	if(fpwm) 
	{
		control |= 0x08;
	}

    if(conv) 
	{
		control |= 0x20;
	}

    if(range) 
	{
		control |= 0x40;
	}

	CHECK_LOGE(dev, write_reg8(&dev->i2c_dev, SGM_CONTROL, control), "Failed to set control register");

	I2C_DEV_GIVE_MUTEX(&dev->i2c_dev);

    return ESP_OK;
}
esp_err_t sgm62110_set_vout1(sgm62110_t *dev, uint8_t vset1)
{
    CHECK_ARG(dev);
	I2C_DEV_TAKE_MUTEX(&dev->i2c_dev);

    uint16_t vout1;

    uint8_t range = get_range_cmd(dev);
    if(range){
        vout1=SGM_MIN_VOUT1_R1+(vset1*SGM_VOUT1_STEP);
        if(vout1<SGM_MIN_VOUT1_R1||vout1>SGM_MAX_VOUT1_R1){
            ESP_LOGE(TAG, "vout out of high range [%d, %d]mV", SGM_MIN_VOUT1_R1, SGM_MAX_VOUT1_R1);
        }else{
            ESP_LOGI(TAG, "Set vout %d mV", vout1);
        }
    }else{
        vout1=SGM_MIN_VOUT1_R0+(vset1*SGM_VOUT1_STEP);
        if(vout1<SGM_MIN_VOUT1_R0||vout1>SGM_MAX_VOUT1_R0){
            ESP_LOGE(TAG, "vout out of low range [%d, %d]mV", SGM_MIN_VOUT1_R0, SGM_MAX_VOUT1_R0);
        }else{
            ESP_LOGI(TAG, "Set vout %d mV", vout1);
        }
    }
    
    CHECK_LOGE(dev, write_reg8(&dev->i2c_dev, SGM_VOUT1, vset1), "Failed to set control register");

	I2C_DEV_GIVE_MUTEX(&dev->i2c_dev);

    return ESP_OK;	
}
esp_err_t sgm62110_set_vout2(sgm62110_t *dev, uint8_t vset2)
{
    CHECK_ARG(dev);
	I2C_DEV_TAKE_MUTEX(&dev->i2c_dev);

    uint16_t vout2;

    uint8_t range = get_range_cmd(dev);
    if(range){
        vout2=SGM_MIN_VOUT2_R1+(vset2*SGM_VOUT2_STEP);
        if(vout2<SGM_MIN_VOUT2_R1||vout2>SGM_MAX_VOUT2_R1){
            ESP_LOGE(TAG, "vout out of high range [%d, %d]mV", SGM_MIN_VOUT2_R1, SGM_MAX_VOUT2_R1);
        }else{
            ESP_LOGI(TAG, "Set vout %d mV", vout2);
        }
    }else{
        vout2=SGM_MIN_VOUT2_R0+(vset2*SGM_VOUT2_STEP);
        if(vout2<SGM_MIN_VOUT2_R0||vout2>SGM_MAX_VOUT2_R0){
            ESP_LOGE(TAG, "vout out of low range [%d, %d]mV", SGM_MIN_VOUT2_R0, SGM_MAX_VOUT2_R0);
        }else{
            ESP_LOGI(TAG, "Set vout %d mV", vout2);
        }
    }

    CHECK_LOGE(dev, write_reg8(&dev->i2c_dev, SGM_VOUT2, vset2), "Failed to set control register");

	I2C_DEV_GIVE_MUTEX(&dev->i2c_dev);

    return ESP_OK;	
}