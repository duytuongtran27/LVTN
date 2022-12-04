/*
 *
 *    Copyright (c) 2022 Project CHIP Authors
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "AppTask.h"
#include "Device.h"
#include "DeviceCallbacks.h"
#include "Globals.h"

#include "hw_config.h"
#include "Common.h"
#include "EEPROM/at24c.h"
#include "IR/IRSend.h"
#include "NVS/NVSDriver.h"

#include "esp_efuse.h"
#include "esp_efuse_table.h"
#include "esp_heap_caps_init.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_spi_flash.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#include <credentials/DeviceAttestationCredsProvider.h>
#include <credentials/examples/DeviceAttestationCredsExample.h>
#include <app-common/zap-generated/af-structs.h>
#include <app-common/zap-generated/attribute-id.h>
#include <app-common/zap-generated/cluster-id.h>
#include <app-common/zap-generated/attributes/Accessors.h>
#include <app/ConcreteAttributePath.h>
#include <app/clusters/identify-server/identify-server.h>
#include <app/reporting/reporting.h>
#include <app/util/attribute-storage.h>
#include <app/InteractionModelEngine.h>
#include <app/server/Server.h>

#include <common/Esp32AppServer.h>
#include <ota/OTAHelper.h>
#include "OTAImageProcessorImpl.h"
#include <lib/core/CHIPError.h>
#include <lib/support/CHIPMem.h>
#include <lib/support/CHIPMemString.h>
#include <lib/support/ErrorStr.h>
#include <lib/support/ZclString.h>

#if CONFIG_ENABLE_ESP32_FACTORY_DATA_PROVIDER
#include <platform/ESP32/ESP32FactoryDataProvider.h>
#endif // CONFIG_ENABLE_ESP32_FACTORY_DATA_PROVIDER

#if CONFIG_ENABLE_ESP32_DEVICE_INFO_PROVIDER
#include <platform/ESP32/ESP32DeviceInfoProvider.h>
#else
#include <DeviceInfoProviderImpl.h>
#endif // CONFIG_ENABLE_ESP32_DEVICE_INFO_PROVIDER

namespace {
  #if CONFIG_ENABLE_ESP32_FACTORY_DATA_PROVIDER
  chip::DeviceLayer::ESP32FactoryDataProvider sFactoryDataProvider;
  #endif // CONFIG_ENABLE_ESP32_FACTORY_DATA_PROVIDER

  #if CONFIG_ENABLE_ESP32_DEVICE_INFO_PROVIDER
  chip::DeviceLayer::ESP32DeviceInfoProvider gExampleDeviceInfoProvider;
  #else
  chip::DeviceLayer::DeviceInfoProviderImpl gExampleDeviceInfoProvider;
  #endif // CONFIG_ENABLE_ESP32_DEVICE_INFO_PROVIDER
} // namespace

using namespace ::chip;
using namespace ::chip::DeviceManager;
using namespace ::chip::Platform;
using namespace ::chip::Credentials;
using namespace ::chip::app::Clusters;

const char * TAG = "ambi-fw-esp32-main";

HWData_t HWData;

static AppDeviceCallbacks EchoCallbacks;

void OTAEventsHandler(const DeviceLayer::ChipDeviceEvent * event, intptr_t arg)
{
    if (event->Type == DeviceLayer::DeviceEventType::kOtaStateChanged)
    {
        switch (event->OtaStateChanged.newState)
        {
        case DeviceLayer::kOtaDownloadInProgress:
            ChipLogProgress(DeviceLayer, "OTA image download in progress");
            break;
        case DeviceLayer::kOtaDownloadComplete:
            ChipLogProgress(DeviceLayer, "OTA image download complete");
            break;
        case DeviceLayer::kOtaDownloadFailed:
            ChipLogProgress(DeviceLayer, "OTA image download failed");
            break;
        case DeviceLayer::kOtaDownloadAborted:
            ChipLogProgress(DeviceLayer, "OTA image download aborted");
            break;
        case DeviceLayer::kOtaApplyInProgress:
            ChipLogProgress(DeviceLayer, "OTA image apply in progress");
            break;
        case DeviceLayer::kOtaApplyComplete:
            ChipLogProgress(DeviceLayer, "OTA image apply complete");
            break;
        case DeviceLayer::kOtaApplyFailed:
            ChipLogProgress(DeviceLayer, "OTA image apply failed");
            break;
        default:
            break;
        }
    }
}

static void InitServer(intptr_t context)
{
    Esp32AppServer::Init();
}
void Read_EEPROM(void)
{
    ESP_LOGI(TAG, "EEPROM is 24C%d", EEPROM_TYPE);

    static at24c_t dev4;

    ESP_ERROR_CHECK(i2cdev_init());

    memset(&dev4, 0, sizeof(at24c_t));
    dev4.size=EEPROM_TYPE;
    ESP_ERROR_CHECK(at24cx_init_desc(&dev4, 0, I2C_SDA, I2C_SCL));

    esp_err_t ret;
 
    ESP_LOGI(TAG, "Read MAC");
    
    for (int i=MAC_OFFSET_EEPROM; i<(MAC_OFFSET_EEPROM+MAC_LEN); i++) {
      uint16_t data_addr = i;
      ret = ReadRom(&dev4, data_addr, &HWData.mac[i-MAC_OFFSET_EEPROM]);
      if (ret != ESP_OK) {
        ESP_LOGI(TAG, "ReadRom fail %d", ret);
        while(1) { vTaskDelay(1); }
      }
    }
    snprintf(HWData.macstr, 18, "%02X:%02X:%02X:%02X:%02X:%02X", HWData.mac[0], HWData.mac[1], HWData.mac[2], HWData.mac[3], HWData.mac[4], HWData.mac[5]);

    ESP_LOGI(TAG, "MAC: %s", HWData.macstr);

    ESP_LOGI(TAG, "Read SN");
    for (int i=SN_OFFSET_EEPROM; i<(SN_OFFSET_EEPROM+SN_LEN); i++) {
      uint16_t data_addr = i;
      ret = ReadRom(&dev4, data_addr, &HWData.sn[i-SN_OFFSET_EEPROM]);
      if (ret != ESP_OK) {
        ESP_LOGI(TAG, "ReadRom fail %d", ret);
        while(1) { vTaskDelay(1); }
      }
    }
    ESP_LOGI(TAG, "SN: %c%c%c%c%c%c%c%c%c%c%c%c", (char)HWData.sn[0],(char)HWData.sn[1],(char)HWData.sn[2],(char)HWData.sn[3],(char)HWData.sn[4],(char)HWData.sn[5],(char)HWData.sn[6],(char)HWData.sn[7], (char)HWData.sn[8],(char)HWData.sn[9],(char)HWData.sn[10],(char)HWData.sn[11]);
    snprintf(HWData.snstr, 13, "%c%c%c%c%c%c%c%c%c%c%c%c", (char)HWData.sn[0],(char)HWData.sn[1],(char)HWData.sn[2],(char)HWData.sn[3],(char)HWData.sn[4],(char)HWData.sn[5],(char)HWData.sn[6],(char)HWData.sn[7], (char)HWData.sn[8],(char)HWData.sn[9],(char)HWData.sn[10],(char)HWData.sn[11]);
    ESP_ERROR_CHECK(at24cx_free_desc(&dev4));
}
extern "C" void app_main()
{
    ESP_LOGI(TAG,"========================RAM left %d==============================", esp_get_free_heap_size());

    led.Init();
    led.Run();
    led.SetColor(LED_AMBI_SECONDARY_HUE, LED_AMBI_SECONDARY_SAT);
    led.SetBrightness(LED_BRIGHTNESS_DEFAULT);
    led.FadeIn();
    
    // Initialize the ESP NVS layer.
    esp_err_t err = nvs_flash_init();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "nvs_flash_init() failed: %s", esp_err_to_name(err));
        return;
    }
    ESP_LOGI(TAG, "==================================================");
    ESP_LOGI(TAG, "AmbiMiniV2 starting");
    ESP_LOGI(TAG, "==================================================");

    ESP_LOGI(TAG, "AmbiMiniV2 Demo");

    ESP_LOGI(TAG, "AmbiMiniV2 Firmware Version %d%d%d", FW_VERSION_MAJOR, FW_VERSION_MINOR, FW_VERSION_BUILD);

    ESP_LOGI(TAG, "AmbiMiniV2 Chip information");

    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    ESP_LOGI(TAG, "This is ESP32 chip with %d CPU cores, WiFi%s%s, ", chip_info.cores,
             (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "", (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");
    ESP_LOGI(TAG, "silicon revision %d, ", chip_info.revision);
    ESP_LOGI(TAG, "%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
             (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    ESP_LOGI(TAG, "AmbiMiniV2 Read EPPROM");
    Read_EEPROM();

    //read uuid from efuse of esp32
    uint8_t uuid[16];
    esp_efuse_read_field_blob(ESP_EFUSE_OPTIONAL_UNIQUE_ID, (void*)uuid, 128);
    std::string uuid_str;
    const char char_table[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
    for(uint8_t i = 0; i < 16; i++)
    {
      uuid_str += char_table[uuid[i] >> 4];
      uuid_str += char_table[uuid[i] & 0x0F];
    }
    ESP_LOGI(TAG, "AmbiMiniV2 read UUID from Efuse: %s",uuid_str.c_str());

    ESP_LOGI(TAG, "AmbiMiniV2 Set Base MAC Address");
    esp_base_mac_addr_set(HWData.mac);

    IRSendInstall(38000, 50);

    DeviceLayer::SetDeviceInfoProvider(&gExampleDeviceInfoProvider);

    CHIPDeviceManager & deviceMgr = CHIPDeviceManager::GetInstance();
    CHIP_ERROR error              = deviceMgr.Init(&EchoCallbacks);
    if (error != CHIP_NO_ERROR)
    {
        ESP_LOGE(TAG, "device.Init() failed: %s", ErrorStr(error));
        return;
    }
    
    ESP_LOGI(TAG,"========================RAM left %d==============================", esp_get_free_heap_size());
    
    #if CONFIG_ENABLE_ESP32_FACTORY_DATA_PROVIDER
    SetCommissionableDataProvider(&sFactoryDataProvider);
    SetDeviceAttestationCredentialsProvider(&sFactoryDataProvider);
#if CONFIG_ENABLE_ESP32_DEVICE_INSTANCE_INFO_PROVIDER
    SetDeviceInstanceInfoProvider(&sFactoryDataProvider);
#endif
#else
    SetDeviceAttestationCredentialsProvider(Examples::GetExampleDACProvider());
#endif // CONFIG_ENABLE_ESP32_FACTORY_DATA_PROVIDER

    ESP_LOGI(TAG, "------------------------Starting App Task---------------------------");
    error = GetAppTask().StartAppTask();
    if (error != CHIP_NO_ERROR)
    {
        ESP_LOGE(TAG, "GetAppTask().StartAppTask() failed : %s", ErrorStr(error));
    }

    chip::DeviceLayer::PlatformMgr().ScheduleWork(InitServer, reinterpret_cast<intptr_t>(nullptr));
    chip::DeviceLayer::PlatformMgrImpl().AddEventHandler(OTAEventsHandler, reinterpret_cast<intptr_t>(nullptr));

    ESP_LOGI(TAG,"========================RAM left %d==============================", esp_get_free_heap_size());
}
