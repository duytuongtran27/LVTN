#include "IRWidget.h"
#include "../NVS/NVSDriver.h"
#include "../include/Common.h"

static const char * TAG = "IRWidget";

void IRWidgetSendRAW(const cJSON * const root){
    int ir_array_size = cJSON_GetArraySize(root);
    ESP_LOGI(TAG, "ir_array_size=%d", ir_array_size);
    uint32_t ir_value[ir_array_size];
    for (int i=0;i<ir_array_size;i++) {
        cJSON *array = cJSON_GetArrayItem(root,i);
        ir_value[i] = array->valueint;
        ESP_LOGI(TAG, "ir_value[%d]=%d",i, ir_value[i]);
    }
    //IRSendInstall(38000, 50);
    IRSendRAW(ir_value, ir_array_size);
}
void IRWidgetRecvSetStartCondition(const cJSON * const root){
    int ir_array_size = cJSON_GetArraySize(root);
    ESP_LOGI(TAG, "ir_array_size=%d", ir_array_size);
    uint32_t ir_value[ir_array_size];
    for (int i=0;i<ir_array_size;i++) {
        cJSON *array = cJSON_GetArrayItem(root,i);
        ir_value[i] = array->valueint;
        ESP_LOGI(TAG, "ir_value[%d]=%d",i, ir_value[i]);
    }    
    
    NVSDriverOpen(NVS_NAMESPACE_CONFIG);
    NVSDriverWriteU32(NVS_KEY_START_COND_MARK, ir_value[0]);
    NVSDriverWriteU32(NVS_KEY_START_COND_SPACE, ir_value[1]);
    NVSDriverClose();

    IRRecvSetStartCondition(ir_value[0], ir_value[1], 200);
}
void IRWidgetRXCreateTask(void){
    IRRecvRXCreateTask();
}
void IRWidgetRAWCreateTask(void){
    IRRecvRAWCreateTask();
}
void IRWidgetSetIRTXPower(const cJSON * const root){
    int ir_array_size = cJSON_GetArraySize(root);
    ESP_LOGI(TAG, "ir_array_size=%d", ir_array_size);
    uint32_t pwr_value[ir_array_size];
    for (int i=0;i<ir_array_size;i++) {
        cJSON *array = cJSON_GetArrayItem(root,i);
        pwr_value[i] = array->valueint;
        ESP_LOGI(TAG, "pwr_value[%d]=%d",i, pwr_value[i]);
    }
    NVSDriverOpen(NVS_NAMESPACE_CONFIG);
    NVSDriverWriteU32(NVS_KEY_IR_TX_POWER, pwr_value[0]);
    NVSDriverClose();
    // static sgm62110_t dev5;
    // memset(&dev5, 0, sizeof(sgm62110_t));
    
    // ESP_LOGI(TAG, "Init SGM62110");
    // ESP_ERROR_CHECK(sgm62110_init_desc(&dev5, 0, I2C_SDA, I2C_SCL));
    // ESP_ERROR_CHECK(sgm62110_init(&dev5));

    // ESP_LOGI(TAG, "Get Device Identify SGM62110");
    // ESP_ERROR_CHECK(sgm62110_get_dev_id(&dev5));

    // ESP_LOGI(TAG, "Get Device Status SGM62110");
    // ESP_ERROR_CHECK(sgm62110_get_status(&dev5));

    // ESP_LOGI(TAG, "Get Param");
    // ESP_ERROR_CHECK(sgm62110_get_param(&dev5));

    // ESP_LOGI(TAG, "Set vout");
    // ESP_ERROR_CHECK(sgm62110_set_vout1(&dev5, pwr_value[0]));
}