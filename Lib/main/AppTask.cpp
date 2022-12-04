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
#include "hw_config.h"
#include "BUZZER/piezo_driver.h"
#include "NVS/NVSDriver.h"
#include "Common.h"

#include "AppTask.h"
#include "Button.h"
#include "Globals.h"

#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_spi_flash.h"
#include "freertos/FreeRTOS.h"
#include "nvs_flash.h"

#include <app/server/OnboardingCodesUtil.h>
#include <app-common/zap-generated/attributes/Accessors.h>
#include <app/server/Server.h>

#define APP_TASK_NAME               "APP"
#define APP_EVENT_QUEUE_SIZE        10
#define APP_TASK_STACK_SIZE         3*1024

static const char * TAG = "ambi-fw-esp32-apptask";
namespace {

    QueueHandle_t sAppEventQueue;
    TaskHandle_t sAppTaskHandle;
} // namespace

Button gButtons[BUTTON_NUMBER] = { Button(BTNRST)};

AppTask AppTask::sAppTask;

CHIP_ERROR AppTask::StartAppTask()
{
    sAppEventQueue = xQueueCreate(APP_EVENT_QUEUE_SIZE, sizeof(AppEvent));
    if (sAppEventQueue == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate app event queue");
        return APP_ERROR_EVENT_QUEUE_FAILED;
    }

    // Start App task.
    BaseType_t xReturned;
    xReturned = xTaskCreate(AppTaskMain, APP_TASK_NAME, APP_TASK_STACK_SIZE, NULL, 1, &sAppTaskHandle);
    return (xReturned == pdPASS) ? CHIP_NO_ERROR : APP_ERROR_CREATE_TASK_FAILED;
}
CHIP_ERROR AppTask::Init()
{
    CHIP_ERROR err = CHIP_NO_ERROR;

    esp_err_t errr;
    // Initialize the buttons.
    errr = gpio_install_isr_service(0);
    ESP_LOGI(TAG,"Button preInit: %s", esp_err_to_name(errr));
    for (int i = 0; i < BUTTON_NUMBER; ++i)
    {
        errr = gButtons[i].Init();
        ESP_LOGI(TAG,"Button.Init(): %s", esp_err_to_name(errr));
    }
    // Print QR Code URL
    PrintOnboardingCodes(chip::RendezvousInformationFlags(CONFIG_RENDEZVOUS_MODE));

    led.Blink(1000);

    piezo_play(NOTE_C8, 7000);
    vTaskDelay(100/portTICK_PERIOD_MS);
    piezo_stop();
    
    return err;
}

void AppTask::AppTaskMain(void * pvParameter)
{
    AppEvent event;
    CHIP_ERROR err = sAppTask.Init();
    if (err != CHIP_NO_ERROR)
    {
        ESP_LOGI(TAG, "AppTask.Init() failed due to %" CHIP_ERROR_FORMAT, err.Format());
        return;
    }

    ESP_LOGI(TAG, "App Task started");

    while (true)
    {
        BaseType_t eventReceived = xQueueReceive(sAppEventQueue, &event, pdMS_TO_TICKS(10));
        while (eventReceived == pdTRUE)
        {
            sAppTask.DispatchEvent(&event);
            eventReceived = xQueueReceive(sAppEventQueue, &event, 0); // return immediately if the queue is empty
        }
    }
}

void AppTask::PostEvent(const AppEvent * aEvent)
{
    if (sAppEventQueue != NULL)
    {
        BaseType_t status;
        if (xPortInIsrContext())
        {
            BaseType_t higherPrioTaskWoken = pdFALSE;
            status                         = xQueueSendFromISR(sAppEventQueue, aEvent, &higherPrioTaskWoken);
        }
        else
        {
            status = xQueueSend(sAppEventQueue, aEvent, 1);
        }
        if (!status)
            ESP_LOGE(TAG, "Failed to post event to app task event queue");
    }
    else
    {
        ESP_LOGE(TAG, "Event Queue is NULL should never happen");
    }
}

void AppTask::DispatchEvent(AppEvent * aEvent)
{
    if (aEvent->mHandler)
    {
        aEvent->mHandler(aEvent);
    }
    else
    {
        ESP_LOGI(TAG, "Event received with no handler. Dropping event.");
    }
}

void AppTask::ButtonEventHandler(uint8_t btnIdx, uint8_t btnAction)
{
    AppEvent button_event             = {};
    button_event.mType                = AppEvent::kEventType_Button;
    button_event.mButtonEvent.mPinNo  = btnIdx;
    button_event.mButtonEvent.mAction = btnAction;

    if (btnAction == APP_BUTTON_PRESSED)
    {
        button_event.mHandler = ButtonPressedAction;
        sAppTask.PostEvent(&button_event);
    }
}

void AppTask::ButtonPressedAction(AppEvent * aEvent)
{
    uint32_t io_num = aEvent->mButtonEvent.mPinNo;
    int cnt=0;
    char* cloudEnv;

    NVSDriverOpen(NVS_NAMESPACE_AWS);
    NVSDriverReadString(NVS_KEY_CLOUD_ENV, &cloudEnv);
    NVSDriverClose();

    while(gpio_get_level((gpio_num_t) io_num) == 0)
    {
        ESP_LOGI(TAG, "gpio_get_level == 0");
        cnt++;
        ESP_LOGW(TAG, "cnt = %d ...", cnt);
        if(cnt==5&&cloudEnv==NULL){
            ESP_LOGW(TAG, "erase nvs partition & reboot");
            chip::Server::GetInstance().ScheduleFactoryReset();
        }else if(cnt==10&&(strcmp(cloudEnv, "STAGING") == 0)){
            NVSDriverOpen(NVS_NAMESPACE_AWS);
            NVSDriverEraseKey(NVS_KEY_CERT_ID);
            NVSDriverEraseKey(NVS_KEY_CERT_PEM);
            NVSDriverEraseKey(NVS_KEY_PRIVATE_KEY);
            NVSDriverClose();
            esp_restart();  
        }
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
}
