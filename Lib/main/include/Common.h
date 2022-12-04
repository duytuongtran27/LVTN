#ifndef __Common__
#define __Common__

#include <stdint.h>
#include "hw_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FW_VERSION_MAJOR         0
#define FW_VERSION_MINOR         2
#define FW_VERSION_BUILD         6

#define NVS_NAMESPACE_AWS        "AWS"
#define NVS_KEY_CERT_ID          "certificateId"
#define NVS_KEY_CERT_PEM         "certificatePem"
#define NVS_KEY_PRIVATE_KEY      "privateKey"
#define NVS_KEY_THING_NAME       "thingName"
#define NVS_KEY_CLOUD_ENV        "cloudEnv"

#define NVS_NAMESPACE_CONFIG     "CONFIG"
#define NVS_KEY_LED_LEVEL        "LL"
#define NVS_KEY_BUZZER_LEVEL     "BL"
#define NVS_KEY_BUZZER_BAND      "BB"
#define NVS_KEY_START_COND_MARK  "FC0"
#define NVS_KEY_START_COND_SPACE "FC1"
#define NVS_KEY_IR_TX_POWER      "IR_TX_POWER"

typedef struct
{
    float tsens_out;
    
    float temperature_sht;
    float humidity_sht;

    float temperature_scd;
    float humidity_scd;
    uint16_t co2;

    float lux;
    uint16_t fspec;
    uint16_t inspec;

    uint8_t signal_quality;
}SensorData_t;

extern SensorData_t SensorData;
typedef struct
{
    uint8_t uid[UID_LEN];
    char uidstr[25];
    uint8_t mac[MAC_LEN];
    char macstr[18];
    uint8_t sn[SN_LEN]; 
    char snstr[13];
    uint8_t pcb_no[PCB_LEN];
    uint8_t hw_var[HW_VAR_LEN];
    uint8_t hw_type[HW_TYPE_LEN];
    uint8_t hw_rev[HW_REV_LEN];
}HWData_t;

extern HWData_t HWData;

extern TaskHandle_t AWSTaskHandle;
extern TaskHandle_t OTATaskHandle;
extern TaskHandle_t LEDTaskHandle;
extern TaskHandle_t SensorTaskHandle;
extern TaskHandle_t LTRTaskHandle;

#ifdef __cplusplus
}
#endif
#endif //__Common__
