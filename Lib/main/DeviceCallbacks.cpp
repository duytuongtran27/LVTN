/*
 *
 *    Copyright (c) 2021 Project CHIP Authors
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
#include "DeviceCallbacks.h"
#include "Globals.h"

#include <app-common/zap-generated/af-structs.h>
#include <app-common/zap-generated/cluster-objects.h>
#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <app-common/zap-generated/attribute-id.h>
#include <app-common/zap-generated/cluster-id.h>
#include <app/AttributeAccessInterface.h>
#include <app/util/attribute-storage.h>
#include <app/server/Dnssd.h>
#include <app/util/util.h>
#include <lib/support/CodeUtils.h>
#include <lib/support/logging/CHIPLogging.h>

#if CONFIG_BT_ENABLED
#include "esp_bt.h"
    #if CONFIG_BT_NIMBLE_ENABLED
        #if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
            #include "esp_nimble_hci.h"
        #endif // ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
        #include "nimble/nimble_port.h"
    #endif // CONFIG_BT_NIMBLE_ENABLED
#endif // CONFIG_BT_ENABLED

#include "SensorExternal/SensorExternal.h"
#include "MQTT/aws.h"
#include "NVS/NVSDriver.h"
#include "Common.h"

#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "route_hook/esp_route_hook.h"

#if CONFIG_ENABLE_OTA_REQUESTOR
#include <ota/OTAHelper.h>
#endif

static const char * TAG = "ambiminiv2-devicecallbacks";

using namespace ::chip;
using namespace ::chip::app;
using namespace ::chip::Inet;
using namespace ::chip::System;
using namespace ::chip::DeviceLayer;

AppDeviceCallbacksDelegate * appDelegate = nullptr;

#if CONFIG_ENABLE_OTA_REQUESTOR
static bool isOTAInitialized = false;
#endif

uint8_t cnt=0;

void HandleRestart(Layer * systemLayer, void * appState)
{
    esp_restart();
}

void AppDeviceCallbacks::PostAttributeChangeCallback(EndpointId endpointId, ClusterId clusterId, AttributeId attributeId,
                                                     uint8_t type, uint16_t size, uint8_t * value)
{
    ESP_LOGI(TAG,
             "PostAttributeChangeCallback - Cluster ID: '" ChipLogFormatMEI
             "', EndPoint ID: '0x%02x', Attribute ID: '" ChipLogFormatMEI "'",
             ChipLogValueMEI(clusterId), endpointId, ChipLogValueMEI(attributeId));

    ESP_LOGI(TAG, "Unhandled cluster ID: " ChipLogFormatMEI, ChipLogValueMEI(clusterId));

    ESP_LOGI(TAG, "Current free heap: %d\n", heap_caps_get_free_size(MALLOC_CAP_8BIT));
}
void AppDeviceCallbacks::DeviceEventCallback(const ChipDeviceEvent * event, intptr_t arg)
{
    switch (event->Type)
    {
        case DeviceEventType::kInternetConnectivityChange:
            OnInternetConnectivityChange(event);
        break;

        case DeviceEventType::kCHIPoBLEConnectionEstablished:
            ESP_LOGI(TAG, "CHIPoBLE connection established");
            led.SetColor(LED_AMBI_SECONDARY_HUE, LED_AMBI_SECONDARY_SAT);
            led.Blink(100);
        break;

        case DeviceEventType::kCHIPoBLEConnectionClosed:
            ESP_LOGI(TAG, "CHIPoBLE disconnected");
            led.SetColor(LED_AMBI_ERROR_HUE, LED_AMBI_ERROR_SAT);
            led.Blink(100);
        break;

        case DeviceEventType::kThreadConnectivityChange:
    #if CONFIG_ENABLE_OTA_REQUESTOR
            if (event->ThreadConnectivityChange.Result == kConnectivity_Established && !isOTAInitialized)
            {
                OTAHelpers::Instance().InitOTARequestor();
                isOTAInitialized = true;
            }
    #endif
        break;

        case DeviceEventType::kCommissioningComplete: 
            {
                ESP_LOGI(TAG, "Commissioning complete");
                led.SetColor(LED_AMBI_PRIMARY_HUE, LED_AMBI_PRIMARY_SAT);
                led.Set(true);
                char* thingName;    
                cnt++;
                ESP_LOGI(TAG, "Commissioning complete cnt %d", cnt);
                NVSDriverOpen(NVS_NAMESPACE_AWS);
                NVSDriverReadString(NVS_KEY_THING_NAME, &thingName);
                NVSDriverClose();
                if(cnt>1&&thingName!=NULL){
                    DeviceLayer::SystemLayer().StartTimer(System::Clock::Milliseconds32(15 * 1000), HandleRestart, nullptr);
                }
                
        #if CONFIG_BT_NIMBLE_ENABLED && CONFIG_USE_BLE_ONLY_FOR_COMMISSIONING

                if (ble_hs_is_enabled())
                {
                    int ret       = nimble_port_stop();
                    esp_err_t err = ESP_OK;
                    if (ret == 0)
                    {
                        nimble_port_deinit();
        #if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
                        err = esp_nimble_hci_and_controller_deinit();
        #endif // ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
                        err += esp_bt_mem_release(ESP_BT_MODE_BTDM);
                        if (err == ESP_OK)
                        {
                            ESP_LOGI(TAG, "BLE deinit successful and memory reclaimed");
                        }
                    }
                    else
                    {
                        ESP_LOGW(TAG, "nimble_port_stop() failed");
                    }
                }
                else
                {
                    ESP_LOGI(TAG, "BLE already deinited");
                }
        #endif
            }
        break;

        case DeviceEventType::kInterfaceIpAddressChanged:
            if ((event->InterfaceIpAddressChanged.Type == InterfaceIpChangeType::kIpV4_Assigned) ||
                (event->InterfaceIpAddressChanged.Type == InterfaceIpChangeType::kIpV6_Assigned))
            {
                chip::app::DnssdServer::Instance().StartServer();
            }
            if (event->InterfaceIpAddressChanged.Type == InterfaceIpChangeType::kIpV6_Assigned)
            {
                ESP_ERROR_CHECK(esp_route_hook_init(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF")));
            }
        break; 
    }

    ESP_LOGI(TAG, "Current free heap: %u\n", static_cast<unsigned int>(heap_caps_get_free_size(MALLOC_CAP_8BIT)));
}
void AppDeviceCallbacks::OnInternetConnectivityChange(const ChipDeviceEvent * event)
{
    appDelegate = AppDeviceCallbacksDelegate::Instance().GetAppDelegate();
    if (event->InternetConnectivityChange.IPv4 == kConnectivity_Established)
    {
        ESP_LOGI(TAG, "IPv4 Server ready...");
        if (appDelegate != nullptr)
        {
            appDelegate->OnIPv4ConnectivityEstablished();
        }
        chip::app::DnssdServer::Instance().StartServer();
#if CONFIG_ENABLE_OTA_REQUESTOR
        if (!isOTAInitialized)
        {
            OTAHelpers::Instance().InitOTARequestor();
            isOTAInitialized = true;
        }
#endif
        led.SetColor(LED_AMBI_PRIMARY_HUE, LED_AMBI_PRIMARY_SAT);
        led.Set(true);

        ESP_LOGI(TAG, "MQTT START");
        AWSRun();

        ESP_LOGI(TAG, "READ TEMPERATURE EXTERNAL START");
        SensorExternalRun();
    }
    else if (event->InternetConnectivityChange.IPv4 == kConnectivity_Lost)
    {
        ESP_LOGE(TAG, "Lost IPv4 connectivity...");
        if (appDelegate != nullptr)
        {
            appDelegate->OnIPv4ConnectivityLost();
        }
        led.SetColor(LED_AMBI_ERROR_HUE, LED_AMBI_ERROR_SAT);
        led.Blink(100);
    }

    if (event->InternetConnectivityChange.IPv6 == kConnectivity_Established)
    {
        ESP_LOGI(TAG, "IPv6 Server ready...");
        chip::app::DnssdServer::Instance().StartServer();

#if CONFIG_ENABLE_OTA_REQUESTOR
        if (!isOTAInitialized)
        {
            OTAHelpers::Instance().InitOTARequestor();
            isOTAInitialized = true;
        }
#endif
    }
    else if (event->InternetConnectivityChange.IPv6 == kConnectivity_Lost)
    {
        ESP_LOGE(TAG, "Lost IPv6 connectivity...");
    }
}
