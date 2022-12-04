/*
 * IRDriver.c
 *
 *  Created on: May 15, 2022
 *      Author: NhatHoang
 */
#include "IRDriver.h"
#include "driver/rmt.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sys/time.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <math.h>

volatile IRState_t ir_state = IR_STATE_UNKNOWN;
IRRAWData_t IR_RAW={0};
IRParamater_t IR_param={0};

char IRRAWString;
uint32_t * IRRAWData;
uint16_t i=0;

TaskHandle_t IRRAW=NULL;
TaskHandle_t IRRX=NULL;

bool IRTXInit(gpio_num_t gpio, rmt_channel_t channel, uint32_t freq, uint32_t duty_percent);
bool IRTXUninstall(rmt_channel_t channel);
bool IRRXUninstall(rmt_channel_t channel);

uint32_t GetUsec(void);
static void BuildItem(rmt_item32_t *item, uint32_t high_us, uint32_t low_us);

bool IRRXMatch(rmt_item32_t item, uint32_t Mark, uint32_t Space, uint32_t Tolerance);
bool IRRXMatchHeader(rmt_item32_t item, uint32_t MarkHeader, uint32_t SpaceHeader, uint32_t Tolerance);
bool IRRXMatchZero(rmt_item32_t item, uint32_t MarkZero, uint32_t SpaceZero, uint32_t Tolerance);
bool IRRXMatchOne(rmt_item32_t item, uint32_t MarkOne, uint32_t SpaceOne, uint32_t Tolerance);

static void IRRXTask(void *pvParameters);
static void IRRAWTask(void *pvParameters);

char* IRDriverGetCaptureStream(void)
{
    if(IRRAWString!=NULL)
        return IRRAWString;
    else
        return 0;
}

uint32_t GetUsec(void){
    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    return tv_now.tv_usec;
}

IRState_t IRDriverGetState(void)
{
        return ir_state;
}

void IRDriverSetStartCondition(uint32_t Mark, uint32_t Space, uint32_t Tolerance){
    ESP_LOGI("IR DRIVER","SetStartCondition");

    IR_param.MarkHeader=Mark;
    IR_param.SpaceHeader=Space;
    IR_param.Tolerance=Tolerance;
}
void IRAM_ATTR IRRAWISRHandler(void* arg) {

    ir_state=IR_STATE_ISR;
    uint32_t thisChange = GetUsec(); 
    if ((thisChange - IR_RAW.lastChange) > 50) {
        IR_RAW.spaceLen = thisChange - IR_RAW.spaceStart ;
        IR_RAW.markLen = thisChange - IR_RAW.markStart - IR_RAW.spaceLen;
        IR_RAW.markStart = thisChange;
        IR_RAW.space[i]=IR_RAW.spaceLen;
        IR_RAW.mark[i]=IR_RAW.markLen;
        IR_RAW.pulse[i]=IR_RAW.pulseCount;
        //spaceStart=0;
        i++;

    } else  {
        IR_RAW.spaceStart = thisChange;
    }
    IR_RAW.pulseCount++;
    IR_RAW.lastChange = thisChange;
}
void IRDriverRAWCreateTask(void)
{
    ESP_LOGI("IR DRIVER","IRDriverRAWCreateTask");
    if(xTaskCreate(IRRAWTask, "IRRAWTask", 3* 1024, NULL, 5, &IRRAW)==pdPASS){
        ESP_LOGI("IR DRIVER","xTaskCreate RAW Success");
    }else{
        ESP_LOGI("IR DRIVER","xTaskCreate RAW Failed");
    }
}
void IRDriverRXCreateTask(void)
{
    ESP_LOGI("IR DRIVER","IRDriverRXCreateTask");
    if(xTaskCreate(IRRXTask, "IRRXTask", 4* 1024, NULL, 5, &IRRX)==pdPASS){
        ESP_LOGI("IR DRIVER","xTaskCreate RX Success");
    }else{
        ESP_LOGI("IR DRIVER","xTaskCreate RX Failed");
    }
}

void IRDriverRAWDeleteTask(void){
    ir_state=IR_STATE_IDLE;
    IRDriverRAWUninstall();
    vTaskDelete(IRRAW);
}

void IRDriverRXDeleteTask(void){
    ir_state=IR_STATE_IDLE;
    IRDriverRXUninstall();
    vTaskDelete(IRRX);
}

void IRDriverRXStart(void)
{
    ESP_LOGI("IR DRIVER","start ir rx receive");
    rmt_rx_start(CHANNEL_IR_RMT_RX, true);
}

void IRDriverRXStop(void)
{
    ESP_LOGI("IR DRIVER","stop ir rx receive");
    rmt_rx_stop(CHANNEL_IR_RMT_RX);
}

bool IRRAWInit(gpio_num_t gpio)
{
    gpio_config_t iraw ;
    memset(&iraw,0,sizeof(gpio_config_t));
    iraw.mode=GPIO_MODE_INPUT;
    iraw.pin_bit_mask= (1ULL << gpio);
    iraw.pull_up_en=GPIO_PULLUP_ENABLE;
    iraw.pull_down_en=GPIO_PULLDOWN_DISABLE;
    iraw.intr_type=GPIO_INTR_NEGEDGE;

    gpio_config(&iraw);

    //gpio_install_isr_service(1);
    if(gpio_isr_handler_add(gpio, IRRAWISRHandler, NULL)==ESP_OK){
        return true;
    }else{
        return false;
    }
}

bool IRRXInit(gpio_num_t gpio, rmt_channel_t channel)
{
    ESP_LOGI("IR DRIVER","IRRXInit gpio = %d--- channel = %d", gpio, channel);
    rmt_config_t rmt_rx_config; 
    rmt_rx_config.channel=(rmt_channel_t)channel;
    rmt_rx_config.gpio_num=(gpio_num_t)gpio;
    rmt_rx_config.rmt_mode=RMT_MODE_RX;//RMT_MODE_RX;
    rmt_rx_config.mem_block_num=1;
    rmt_rx_config.rx_config.filter_en=1;
    rmt_rx_config.rx_config.filter_ticks_thresh=100;
    rmt_rx_config.rx_config.idle_threshold=65535;
    rmt_rx_config.clk_div=CLK_DIV;
    if(rmt_config(&rmt_rx_config)==ESP_OK){
        return true;
    }
    else{
        return false;
    }
}

bool IRTXInit(gpio_num_t gpio, rmt_channel_t channel, uint32_t freq, uint32_t duty_percent)
{
	ESP_LOGI("IR DRIVER","IRTXInit freq = %d--- duty = %d", freq, duty_percent);
    rmt_config_t rmt_tx_config;
    rmt_tx_config.channel = (rmt_channel_t)channel;
    rmt_tx_config.gpio_num = (gpio_num_t)gpio;
    rmt_tx_config.mem_block_num = 2;
    rmt_tx_config.clk_div = CLK_DIV;
    rmt_tx_config.tx_config.loop_en = false;
    rmt_tx_config.tx_config.carrier_duty_percent = duty_percent;
    rmt_tx_config.tx_config.carrier_freq_hz = freq;
    rmt_tx_config.tx_config.carrier_level = (rmt_carrier_level_t)1;
    rmt_tx_config.tx_config.carrier_en = 1;
    rmt_tx_config.tx_config.idle_level = (rmt_idle_level_t)0;
    rmt_tx_config.tx_config.idle_output_en = true;
    rmt_tx_config.rmt_mode = RMT_MODE_TX;//RMT_MODE_TX;
    rmt_config(&rmt_tx_config);
    if(rmt_driver_install(rmt_tx_config.channel, 0, 0)==ESP_OK)
    {
        return true;
    }
    else{
        return false;
    }
}

static void BuildItem(rmt_item32_t *item, uint32_t high_us, uint32_t low_us)
{
    item->level0 = 1;
    item->duration0 = high_us;
    item->level1 = 0;
    item->duration1 = low_us;
}

bool IRRXMatch(rmt_item32_t item, uint32_t Mark, uint32_t Space, uint32_t Tolerance) 
{
	uint32_t lowValue = item.duration0 * 10 / TICK_10_US;
	uint32_t highValue = item.duration1 * 10 / TICK_10_US;

	ESP_LOGI("IR Driver", "lowValue=%d, highValue=%d, Mark=%d, Space=%d", lowValue, highValue, Mark, Space);
	if (lowValue < (Mark - Tolerance) || lowValue > (Mark + Tolerance) ||
    (highValue != 0 &&(highValue < (Space - Tolerance) || highValue > (Space + Tolerance)))) 
    {
		return false;
	}
	return true;
}
bool IRRXMatchHeader(rmt_item32_t item, uint32_t MarkHeader, uint32_t SpaceHeader, uint32_t Tolerance){
    return IRRXMatch(item, MarkHeader, SpaceHeader, Tolerance);
}
bool IRRXMatchZero(rmt_item32_t item, uint32_t MarkZero, uint32_t SpaceZero, uint32_t Tolerance){
    return IRRXMatch(item, MarkZero, SpaceZero, Tolerance);
}
bool IRRXMatchOne(rmt_item32_t item, uint32_t MarkOne, uint32_t SpaceOne, uint32_t Tolerance){
    return IRRXMatch(item, MarkOne, SpaceOne, Tolerance);
}

void IRDriverSendRAW(uint32_t *IRdata, uint16_t IRlength)
{
    uint16_t i = 0;
    uint16_t x = 0;
    uint16_t size = (sizeof(rmt_item32_t) * IRlength);
    rmt_item32_t *item = (rmt_item32_t *)malloc(size); 
    memset((void *)item, 0, size);                     
    ESP_LOGI("IR DRIVER","Sending.....: %d", IRlength);
    while (x < IRlength)
    {
        printf("%d", IRdata[x]);
        printf(",");
        printf("%d", IRdata[x + 1]);
        printf(",");
        BuildItem(&item[i], IRdata[x], IRdata[x + 1]);
        x = x + 2;
        i++;
    }
    printf("\n");
    ESP_LOGI("IR","End");
    rmt_write_items(CHANNEL_IR_RMT_TX, item, IRlength, true);
    rmt_wait_tx_done(CHANNEL_IR_RMT_TX, 100);
    free(item);
    //IRDriverTXUninstall();
}
void IRDriverSendMarkSpace(uint32_t mask, uint32_t space)
{
    uint8_t size = (sizeof(rmt_item32_t) * (1));
    rmt_item32_t *item = (rmt_item32_t *)malloc(size);
    memset((void *)item, 0, size);

    item->level0 = 1;
    item->duration0 = (mask) / 10 * TICK_10_US;
    item->level1 = 0;
    if( space > 0x7FFF) item->duration1 = (0x7FFF) / 10 * TICK_10_US;
    else
    	item->duration1 = (space) / 10 * TICK_10_US;

    uint8_t item_num = 1 * 1;

    rmt_write_items(CHANNEL_IR_RMT_TX, item, item_num, false);
    rmt_wait_tx_done(CHANNEL_IR_RMT_TX, 50000);
    free(item);
    //IRDriverTXUninstall();
}
void IRDriverSendMark(uint32_t mask)
{
    uint8_t size = (sizeof(rmt_item32_t) * (1));
    rmt_item32_t *item = (rmt_item32_t *)malloc(size);
    memset((void *)item, 0, size);

    if( mask > 0x7FFF)mask = 0x7FFF;
	item->level0 = 0;
	item->level1 = 1;
	item->duration0 = (mask) / 10 * TICK_10_US;
    item->duration1 = (mask) / 10 * TICK_10_US;

    uint8_t item_num = 1 * 1;

    rmt_write_items(CHANNEL_IR_RMT_TX, item, item_num, false);
    rmt_wait_tx_done(CHANNEL_IR_RMT_TX, 200);

    free(item);
    //IRDriverTXUninstall();
}

void IRDriverSendSpace(uint32_t space)
{
    uint8_t size = (sizeof(rmt_item32_t) * (1));
    rmt_item32_t *item = (rmt_item32_t *)malloc(size);
    memset((void *)item, 0, size);

    if( space > 0x7FFF)space = 0x7FFF;
	item->level0 = 1;
	item->level1 = 0;
	item->duration0 = (space) / 10 * TICK_10_US;
    item->duration1 = (space) / 10 * TICK_10_US;

    uint8_t item_num = 1 * 1;

    rmt_write_items(CHANNEL_IR_RMT_TX, item, item_num, false);
    rmt_wait_tx_done(CHANNEL_IR_RMT_TX, 200);
    free(item);
    //IRDriverTXUninstall();
}
void IRDriverRAWInstall(void){
    if(IRRAWInit(IR_GPIO_RAW)){
        ESP_LOGI("IR DRIVER","IRDriverRAWInstall Successfully");
    }
    else{
        ESP_LOGI("IR DRIVER","IRDriverRAWInstall Failed");
    }
}
void IRDriverRXInstall(void)
{
    if(IRRXInit(IR_GPIO_RX,CHANNEL_IR_RMT_RX)){
        ESP_LOGI("IR DRIVER","IRDriverRXInstall Successfully");
    }
    else{
        ESP_LOGI("IR DRIVER","IRDriverRXInstall Failed");
    }
}
void IRDriverTXInstall(uint32_t freq, uint32_t duty_percent)
{
    if(IRTXInit(IR_GPIO_TX,CHANNEL_IR_RMT_TX,freq,duty_percent)){
        ESP_LOGI("IR DRIVER","IRDriverTXInstall Successfully");
    }
    else{
        ESP_LOGI("IR DRIVER","IRDriverTXInstall Failed");
    }
}
void IRDriverRAWUninstall(void){
    if(gpio_isr_handler_remove(IR_GPIO_RAW)==ESP_OK){
        ESP_LOGI("IR DRIVER","IRDriverRAWUninstall Successfully");
    }else{
        ESP_LOGI("IR DRIVER","IRDriverRAWUninstall Successfully");
    }
}
void IRDriverRXUninstall(void)
{
    if(IRRXUninstall(CHANNEL_IR_RMT_RX)){
        ESP_LOGI("IR DRIVER","IRDriverRXUninstall Successfully");
    }
    else{
        ESP_LOGI("IR DRIVER","IRDriverRXUninstall Failed");
    }
}

void IRDriverTXUninstall(void)
{
    if(IRTXUninstall(CHANNEL_IR_RMT_TX)){
        ESP_LOGI("IR DRIVER","IRTXUninstall Successfully");
    }
    else{
        ESP_LOGI("IR DRIVER","IRTXUninstall Failed");
    }
}
bool IRRXUninstall(rmt_channel_t channel)
{
	ESP_LOGI("IR DRIVER","IRRXUninstall = %d", channel);
	rmt_rx_stop((rmt_channel_t)channel);
    if (rmt_driver_uninstall(channel) == ESP_OK)
        return true;
    else
        return false;
}
bool IRTXUninstall(rmt_channel_t channel)
{
	ESP_LOGI("IR DRIVER","IRTXUninstall = %d", channel);
	rmt_tx_stop((rmt_channel_t)channel);
    if (rmt_driver_uninstall(channel) == ESP_OK)
        return true;
    else
        return false;
}
static void IRRAWTask(void *pvParameters){
    ESP_LOGI("IR DRIVER","Start IRRAWTask");
    ir_state=IR_STATE_RAW;
    IRDriverRAWInstall();
    while(1){  
        ESP_LOGI("IR DRIVER", "Stack remaining for task '%s' is %d bytes", pcTaskGetName(IRRAW), uxTaskGetStackHighWaterMark(IRRAW));
        switch (ir_state)
        {
            case IR_STATE_RAW:
                ESP_LOGI("IR DRIVER","IR RAW Waiting IR Signal");
                break;
            case IR_STATE_ISR:
                if(IR_RAW.mark[1]>0){
                    uint16_t index =0;
                    //IRRAWString=(char*)malloc(2048*sizeof(char));
                    IRRAWData=(uint32_t*)calloc(1024, sizeof(uint32_t));
                    //memset(IRRAWString,0,sizeof(IRRAWString));
                    for(int j=1;j<=i;j++){
                        IRRAWData[index]=IR_RAW.mark[j];
                        printf("%d,",IR_RAW.mark[j]);
                        IRRAWData[index+1]=IR_RAW.space[j];
                        printf("%d,",IR_RAW.space[j]);
                        index=index+2;
                    }
                    printf("\nTotal bit: %d\n",i);
                    //index=0;
                    // for (int k = 0; k < (i*2); k++) {
                    //     index += sprintf (&IRRAWString[index], "%d,", IRRAWData[k]);
                    // }
                    printf("Frequency :%d(Hz)\n",(IR_RAW.pulse[1]*1000000)/IR_RAW.mark[1]);
                    //printf ("\nIR RAW: %s\n\n", IRRAWString);
                    // free(IRRAWString);
                    free(IRRAWData);
                    ir_state=IR_STATE_IDLE;

                }
                break;
            default:
                    i=0;
                    IR_RAW.pulseCount=0;
                    memset(IR_RAW.pulse,0,sizeof(IR_RAW.pulse));
                    memset(IR_RAW.mark,0,sizeof(IR_RAW.mark));
                    memset(IR_RAW.space,0,sizeof(IR_RAW.space));
                    IRDriverRAWDeleteTask();
                break;
        }
        vTaskDelay(100);
    }
}
static void IRRXTask(void *pvParameters)
{
    ESP_LOGI("IR DRIVER","Start IRRXTask");
    vTaskDelay(10);
    size_t length = 0;
    rmt_item32_t *items = NULL;
    RingbufHandle_t rb = NULL;
    
    IRDriverRXInstall();
    rmt_driver_install(CHANNEL_IR_RMT_RX, 5000, 1);
    //get RMT RX ringbuffer
    rmt_get_ringbuf_handle(CHANNEL_IR_RMT_RX, &rb);
    // Start receive
    IRDriverRXStart();
    rmt_set_source_clk(CHANNEL_IR_RMT_RX, RMT_BASECLK_APB);
    while(1){
        if (rb)
        {
            ESP_LOGI("IR DRIVER", "Stack remaining for task '%s' is %d bytes", pcTaskGetName(IRRX), uxTaskGetStackHighWaterMark(IRRX));
            ESP_LOGI("IR DRIVER","IR RX Waiting IR Signal");
            items = (rmt_item32_t *)xRingbufferReceive(rb, &length, 1000);
            uint16_t numItems = length / sizeof(rmt_item32_t);
            if (items && (numItems >10))
            {
                ESP_LOGI("IR DRIVER","IR RX:");
                ESP_LOGI("IR DRIVER","numItems=%u",numItems);
                uint16_t index =0;
                //IRRAWString=(char*)malloc(2048*sizeof(char));
                IRRAWData=(uint32_t*)calloc(1024, sizeof(uint32_t));
                //memset(IRRAWString,0,sizeof(IRRAWString));
                for (int y=0; y<numItems; y++) {  
                    unsigned int Mark = (items[y].duration0) * (10 / TICK_10_US);
                    //Mark = roundTo*round((float)Mark/roundTo);
                    IRRAWData[index]=Mark;
                    printf("%d,",Mark);
                    unsigned int Space = (items[y].duration1) * (10 / TICK_10_US);
                    //Space = roundTo * round((float)Space/roundTo);
                    IRRAWData[index+1]=Space;
                    printf("%d,",Space);
                    index=index+2;
                }
                // index=0;
                // for (int k = 0; k <numItems*2; k++) {
                //     index += sprintf (&IRRAWString[index], "%d,", IRRAWData[k]);
                // }
                //printf ("\nIR RX: %s\n\n", IRRAWString);
                // free(IRRAWString);
                free(IRRAWData);

                if(IRRXMatchHeader(items[0],IR_param.MarkHeader,IR_param.SpaceHeader,IR_param.Tolerance)){
                    ESP_LOGI("IR DRIVER","IR Signal Receive is match with profile");
                    piezo_play(NOTE_C8,7000);
                    vTaskDelay(100/portTICK_PERIOD_MS);                                                           
                    piezo_stop();
                    //IRDriverRXStop();
                }
                vRingbufferReturnItem(rb, (void *)items);
                //IRDriverRXDeleteTask();
            }
            vTaskDelay(1);
        }
    }
}
