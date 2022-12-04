#ifndef __HWCONFIG__
#define __HWCONFIG__

#ifdef __cplusplus
extern "C" {
#endif

#define UID_OFFSET_EEPROM               0
#define UID_LEN                         24

#define MAC_OFFSET_EEPROM               25
#define MAC_LEN                         6

#define SN_OFFSET_EEPROM                32
#define SN_LEN                          12

#define PCB_NO_OFFSET_EEPROM            44
#define PCB_LEN                         10

#define HW_VAR_OFFSET_EEPROM            54
#define HW_VAR_LEN                      4

#define HW_TYPE_OFFSET_EEPROM           58
#define HW_TYPE_LEN                     1

#define HW_REV_OFFSET_EEPROM            59
#define HW_REV_LEN                      4

#define PIEZO_PIN                       GPIO_NUM_1

#define LED_DATA                        GPIO_NUM_10 //whitepcb
//#define LED_DATA                        GPIO_NUM_3

#define I2C_SDA                         GPIO_NUM_4
#define I2C_SCL                         GPIO_NUM_5

#define IR_TX_PIN                       GPIO_NUM_6
#define IR_RX_PIN                       GPIO_NUM_7
#define IR_RAW_PIN                      GPIO_NUM_0

#define BTNRST                          GPIO_NUM_9

#define EEPROM_TYPE                     128 //whitepcb
//#define EEPROM_TYPE                     2
#ifdef __cplusplus
}
#endif
#endif //__HWCONFIG__

