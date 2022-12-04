#ifndef __OTA__
#define __OTA__

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "cJSON.h"

#ifdef __cplusplus
extern "C" {
#endif

void OTATask(void *arg);
void OTARun(const cJSON * const root);

#ifdef __cplusplus
}
#endif

#endif //__OTA__