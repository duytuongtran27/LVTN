#ifndef __NVSDriver__
#define __NVSDriver__

#include "nvs_flash.h"

#ifdef __cplusplus
extern "C" {
#endif

void NVSDriverInit();
void NVSDriverOpen(const char* paramname);
void NVSDriverClose();
esp_err_t NVSDriverCommit();
esp_err_t NVSDriverReadString(const char* paramname, char **out);
esp_err_t NVSDriverWriteString(const char* paramname, char *val);
esp_err_t NVSDriverReadU32(const char* paramname, uint32_t *out);
esp_err_t NVSDriverWriteU32(const char* paramname, uint32_t val);
esp_err_t NVSDriverEraseKey(const char* paramname);
esp_err_t NVSDriverEraseAll();

#ifdef __cplusplus
}
#endif

#endif //__NVSDriver__