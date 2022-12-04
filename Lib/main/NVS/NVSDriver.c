#include "NVSDriver.h"
#include "nvs.h"
#include "esp_log.h"
#include "string.h"

nvs_handle AmbiHandle;

static const char* TAG = "NVS Driver";

void NVSDriverInit(){
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
}

void NVSDriverOpen(const char* paramname)
{
    nvs_open(paramname, NVS_READWRITE, &AmbiHandle);
}

void NVSDriverClose()
{
    nvs_close(AmbiHandle);
}

esp_err_t NVSDriverCommit()
{
    esp_err_t err = nvs_commit(AmbiHandle);
    return err;
}

esp_err_t NVSDriverReadString(const char* paramname, char**out)
{
    size_t required_size;
    esp_err_t err = nvs_get_str(AmbiHandle, paramname, NULL, &required_size);
    if(err==ESP_OK)
    {
        *out =(char*) malloc(required_size);
        if(!out) {ESP_LOGE(TAG, "malloc fail"); return ESP_FAIL;}
        nvs_get_str(AmbiHandle, paramname, *out, &required_size);
        ESP_LOGI(TAG, "load param=%s val=%s", paramname, *out);
    }
    else
    {
        *out=NULL;
        ESP_LOGI(TAG, "load param=%s err=%s", paramname, esp_err_to_name(err));
    }
    //free(*out);
    return err;
}

esp_err_t NVSDriverWriteString(const char* paramname, char *val)
{
    esp_err_t err = nvs_set_str(AmbiHandle, paramname, val);
    if(err==ESP_OK)
    {
        ESP_LOGI(TAG, "save param=%s val=%s", paramname, val);
    } 
    else
    {
        ESP_LOGI(TAG, "save param=%s err=%s", paramname, esp_err_to_name(err));
    }
    return err;
}

esp_err_t NVSDriverReadU32(const char* paramname, uint32_t *out)
{
    esp_err_t err = nvs_get_u32(AmbiHandle, paramname, out);
    if(err==ESP_OK)
    {
        ESP_LOGI(TAG, "save param=%s val=%d", paramname, out);
    } 
    else
    {
        ESP_LOGI(TAG, "save param=%s err=%s", paramname, esp_err_to_name(err));
    }
    return err;
}

esp_err_t NVSDriverWriteU32(const char* paramname, uint32_t val)
{
    esp_err_t err = nvs_set_u32(AmbiHandle, paramname, val);
    if(err==ESP_OK)
    {
        ESP_LOGI(TAG, "save param=%s val=%d", paramname, val);
    } 
    else
    {
        ESP_LOGI(TAG, "save param=%s err=%s", paramname, esp_err_to_name(err));
    }
    return err;
}

esp_err_t NVSDriverEraseKey(const char* paramname)
{
    esp_err_t err = nvs_erase_key(AmbiHandle, paramname);
    if(err==ESP_OK)
    {
        ESP_LOGI(TAG, "erase param=%s", paramname);
    } 
    else
    {
        ESP_LOGI(TAG, "erase param=%s err=%s", paramname, esp_err_to_name(err));
    }
    return err;
}

esp_err_t NVSDriverEraseAll()
{
    esp_err_t err = nvs_erase_all(AmbiHandle);
    if(err==ESP_OK)
    {
        ESP_LOGI(TAG, "erase all");
    } 
    else
    {
        ESP_LOGI(TAG, "erase all err=%s", esp_err_to_name(err));
    }
    return err;
}