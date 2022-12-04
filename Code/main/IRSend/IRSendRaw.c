#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/rmt.h"
#include "IRSendRaw.h"

static const char* TAG = "IR Send";		
const rmt_channel_t TX_CHANNEL = RMT_CHANNEL_0;
const gpio_num_t TX_PIN = GPIO_NUM_2;
static uint16_t raw[] = 
{
    3694, 1478, 518, 1314,458, 1294,478, 582, 
    458, 586, 458, 586, 458, 1314,458, 582, 
    458, 586, 458, 1310,458, 1294,478, 586, 
    458, 1314,458, 586, 454, 606, 438, 1314, 
    458, 1290,482, 582, 458, 1314,458, 1294, 
    478, 562, 478, 586, 458, 1290,482, 586, 
    454, 586, 458, 1294,478, 582, 458, 570, 
    474, 566, 478, 582, 458, 582, 458, 586, 
    458, 558, 482, 562, 478, 590, 454, 566, 
    478, 602, 438, 602, 442, 562, 478, 582, 
    458, 586, 458, 582, 458, 562, 482, 582, 
    458, 562, 482, 582, 458, 1314,458, 602, 
    438, 602, 438, 1334,438, 1294,478, 606, 
    434, 586, 458, 562, 478, 586, 458, 562, 
    478, 586, 458, 1310,462, 582, 458, 562, 
    478, 1290,482, 582, 458, 590, 454, 558, 
    482, 586, 458, 566, 474, 1318,454, 562, 
    478, 1290,482, 586, 458, 562, 478, 582, 
    462, 582, 458, 558, 482, 582, 458, 566, 
    478, 586, 454, 562, 482, 562, 478, 586, 
    458, 586, 458, 578, 462, 562, 478, 582, 
    462, 558, 482, 582, 458, 562, 482, 558, 
    482, 582,458, 0
};
static uint16_t DataLength = sizeof(raw)/sizeof(uint16_t);

void rmt_tx_init() 
{
	//put your setup code here, to run once:
	rmt_config_t rmtConfig;

	rmtConfig.rmt_mode = RMT_MODE_TX;  								//transmit mode
	rmtConfig.channel = TX_CHANNEL;  								//channel to use 0 - 7
	rmtConfig.clk_div = 80;  										//clock divider 1 - 255. source clock is 80MHz -> 80MHz/80 = 1MHz -> 1 tick = 1 us
	rmtConfig.gpio_num = TX_PIN; 									//pin to use
	rmtConfig.mem_block_num = 1; 									//memory block size

	rmtConfig.tx_config.loop_en = 0; 								//no loop
	rmtConfig.tx_config.carrier_freq_hz = 38000;  					//38kHz carrier frequency
	rmtConfig.tx_config.carrier_duty_percent = 50; 					//duty
	rmtConfig.tx_config.carrier_level =  RMT_CARRIER_LEVEL_HIGH; 	//carrier level
	rmtConfig.tx_config.carrier_en = 1;  			                //carrier enable
	rmtConfig.tx_config.idle_level =  RMT_IDLE_LEVEL_LOW ; 			//signal level at idle
	rmtConfig.tx_config.idle_output_en = 1;  					    //output if idle

    esp_err_t err = rmt_set_source_clk(TX_CHANNEL, RMT_BASECLK_APB);
    if(err != ESP_OK){
        ESP_LOGE(TAG,"rmt_set_source_clk failed%d\n", err);
        return;
    }
	err = rmt_config(&rmtConfig);
    if (err != ESP_OK )
    {
        ESP_LOGE(TAG,"rmt_config failed%d\n", err);
        return;
    }
    err = rmt_driver_install(rmtConfig.channel, 0, 0);
    if (err != ESP_OK )
    {
        ESP_LOGE(TAG,"rmt_driver_install failed%d\n", err);
        return;
    }
    ESP_LOGI(TAG,"tx config success\n");
}

void ir_tx_send()
{
    vTaskDelay(10);
    // esp_log_level_set(TAG, ESP_LOG_INFO);

	const int sendDataLength = DataLength/2;        //sendDataLength is the range of the array powerOn and powerOff It is divided by two
													//because each index of object sendDataOn holds High and Low values and the powerOn/powerOff
													//array uses an index for High and other for Low
													
	rmt_item32_t *sendData;
    sendData = (rmt_item32_t *)malloc(sendDataLength * sizeof(rmt_item32_t));

	for(int i = 0, t = 0; i < (sendDataLength*2)-1; i += 2, t++)
	{
		sendData[t].duration0 = raw[i];		        //Odd bit High
		sendData[t].level0 = 1;						
		sendData[t].duration1 = raw[i+1];             //Even bit Low	
		sendData[t].level1 = 0;
	}

    esp_err_t err = rmt_write_items(TX_CHANNEL, sendData, sendDataLength, false);
    if (err != ESP_OK )
    {
        printf ("rmt_write_items failed%d\n", err);
        return;
    }

    // rmt_wait_tx_done(TX_CHANNEL, portMAX_DELAY);

    printf("\n%d bits sent\n" , DataLength);
    // for (int i=0; i<=sendDataLength; i++){
    //     printf( "%i %i ", sendData[i].duration0, sendData[i].duration1);
    // }
    free(sendData);
}