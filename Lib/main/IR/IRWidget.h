#ifndef __IRWidget__
#define __IRWidget__
#include "cJSON.h"
#include "esp_log.h"
#include "IRSend.h"
#include "IRRecv.h"

#ifdef __cplusplus
extern "C" {
#endif

void IRWidgetSendRAW(const cJSON * const root);
void IRWidgetRecvSetStartCondition(const cJSON * const root);
void IRWidgetRXCreateTask(void);
void IRWidgetRAWCreateTask(void);
void IRWidgetSetIRTXPower(const cJSON * const root);
#ifdef __cplusplus
}
#endif

#endif //__IRWidget__