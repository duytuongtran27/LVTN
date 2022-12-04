#include "JSON.h"
#include "Common.h"
#include "Globals.h"

#include "esp_log.h"    
#include <string.h>
#include "cJSON.h"
#include "../BUZZER/piezo_driver.h"
#include "../NVS/NVSDriver.h"
#include "../OTA/OTA.h"
#include "../IR/IRWidget.h"
static const char * TAG = "JSON";
enum 
{ 
	UNKNOWN_CMD,
	ECHO_HOST,
	RESET,
	IR_TX,
	IR_TX_PWR,
	IR_RX,
	IR_RAW,
	FU,
	FC,
	RZ,
	DS,
	SO,
	SN,
	LL,
	BL,
	BB,
	CE,
};

char *CMD_STRINGS[] = {
	"  ", // UNKNOWN_CMD
	"EH", // ECHO_HOST
	"RH", // RESET
	"TX", //IR_TX
	"TX_PWR", //IR_TX_PWR
	"RX", //IR_RX
	"RAW",//IR_RAW
	"FU", //Firmware update
	"FC", //Set start conditon
	"RZ",
	"DS",
	"SO",
	"SN",
	"LL", //Set Led Level
	"BL", //Set Buzzer Level
	"BB", //Set Buzzer Band
	"CE", //Cloud Environment
};

#define __NUMBER_OF_CMD_STRINGS (sizeof(CMD_STRINGS) / sizeof(*CMD_STRINGS))

cJSON *sub;
cJSON *body;

uint8_t devsGetCmd(char *text)
{
	uint8_t i = 0;
	for (i = 0; i < __NUMBER_OF_CMD_STRINGS; i++) {
		if (strcmp(text, CMD_STRINGS[i]) == 0) {
			return i;
		}
	}
	return UNKNOWN_CMD;
}

uint8_t devsParseCmd(char *text)
{
	uint8_t cmd_index = devsGetCmd(text);
    switch (cmd_index) {
		case ECHO_HOST:
			ESP_LOGI(TAG, "ECHO_HOST");
			break;
		case RESET: 
			ESP_LOGI(TAG, "RESET");
			esp_restart();
			break;
		case IR_TX:
			ESP_LOGI(TAG, "IR_TX");
			IRWidgetSendRAW(body);
			break;
		case IR_TX_PWR:
			ESP_LOGI(TAG, "IR_TX_PWR");
			IRWidgetSetIRTXPower(body);
			break;
		case IR_RX:
			ESP_LOGI(TAG, "IR_RX");
			IRWidgetRXCreateTask();
			break;
		case IR_RAW:
			ESP_LOGI(TAG, "IR_RAW");
			IRWidgetRAWCreateTask();
			break;
		case FU:
			ESP_LOGI(TAG, "FU");
			OTARun(sub);
			break;
		case FC:
			ESP_LOGI(TAG, "FC");
			IRWidgetRecvSetStartCondition(body);
			break;
		case RZ:
			ESP_LOGI(TAG, "RZ");
			//IR.GetCaptureStream();
			break;
		case DS:
			ESP_LOGI(TAG, "DS");
			// uiUpdate(body);
			// copyParamToCurrParam();
			// if (hisActUpdate() == 1) { // == 1: need to send IR
			// 	irpUpdate();
			// }
			//lastICorDSParamUpdate(backup, 0);
			break;
		case SO:
			ESP_LOGI(TAG, "SO");
			//irpConditionWrite(body);
			break;
		case SN:
			ESP_LOGI(TAG, "SN");
			//irpDefinitionMapWrite(body);
			break;
		case LL:
			ESP_LOGI(TAG, "LL");
			led.SetLedBrightness(body);
			break;
		case BL:
			ESP_LOGI(TAG, "BL");
			piezo_set_level(body);
			break;
		case BB:
			ESP_LOGI(TAG, "BB");
			piezo_set_band(body);
			break;
		case CE:
			ESP_LOGI(TAG, "CE");
			NVSDriverOpen(NVS_NAMESPACE_AWS);
			NVSDriverWriteString(NVS_KEY_CLOUD_ENV, "STAGING");
			NVSDriverClose();
			break;
		case UNKNOWN_CMD:
			ESP_LOGI(TAG, "UNKNOWN_CMD");
			return 1;
			break;
	}
	return 0;
}

void JSON_Deserialize(const char *value)
{
    
    sub = cJSON_Parse((char *)value);

	body = cJSON_GetObjectItem(sub,"body");

	cJSON *command =cJSON_GetObjectItem(sub,"command");
    if(command){
        char *value_type_cmd =cJSON_GetObjectItem(sub,"command")->valuestring;
        ESP_LOGI(TAG, "command is %s",value_type_cmd);
		devsParseCmd(value_type_cmd);
    }
    cJSON_Delete(sub);
}
