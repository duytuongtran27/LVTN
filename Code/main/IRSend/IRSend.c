#include <stdio.h>
#include <string.h>
#include "esp_err.h"
#include "esp_log.h"
#include "driver/rmt.h"
#include "IRSend.h"
#include "Daikin.h"

static const char* TAG = "IR Send";

const rmt_channel_t TX_CHANNEL = RMT_CHANNEL_0;
const gpio_num_t TX_PIN = GPIO_NUM_2;			

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
	rmtConfig.tx_config.carrier_freq_hz = CARRIER_FREQ;  					//38kHz carrier frequency
	rmtConfig.tx_config.carrier_duty_percent = DUTY_PERCENT; 					//duty
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

void ir_tx_send(){
    ESP_LOGI(TAG,"IR Send Start\n");

// //--------set mode--------------
//     Daikin_setSwing(0);
//     Daikin_setSwingLR(0);

//     //   0 FAN 1 COOL 2 DRY 3 HEAT
//     Daikin_setMode(1);
//     // 0~4 speed,5 auto,6 moon
//     Daikin_setFan(5);
//     // temparature
//     Daikin_setTemp(24);
//     Daikin_setPower(1);

// ------------create tx frame----------
    rmt_item32_t *rmtdata;
    rmtdata = (rmt_item32_t *)malloc(
    (
    wakedatalength + 
    trailerlength  + 
    headerlength   + 
    frame1bytelength * 8 +
    trailerlength  + 
    headerlength   +
    frame2bytelength * 8 +        
    endframelength 
    ) * sizeof(rmt_item32_t));


//  set frame 1 checksum at the end of frame
    uint8_t sum = 0;
	for(uint8_t i = DAIKIN_FRAME1_OFFSET; i < DAIKIN_FRAME1_LENGTH - 1 ; i++){
		sum += daikin[i];
	}
        daikin[DAIKIN_FRAME1_LENGTH - 1] = sum &0xFF;


//  set frame 2 checksum at the end of frame
    sum = 0;
	for(uint8_t i = DAIKIN_FRAME2_OFFSET; i < DAIKIN_FRAME2_LENGTH - 1 ; i++){
		sum += daikin[i];
	}
        daikin[DAIKIN_FRAME2_LENGTH - 1] = sum &0xFF;


//  create frame structure

    uint16_t frameoffset = 0 ;

// create wake signal

    for (int i = 0 ; i < 5 ; i++ ){
        rmtdata[i].duration0 = DAIKIN_ZERO_MARK;
        rmtdata[i].level0 = 1;            
        rmtdata[i].duration1 = DAIKIN_ZERO_SPACE;
        rmtdata[i].level1 = 0;
        frameoffset ++ ;
    }

// create trailer signal

        rmtdata[frameoffset].duration0 = DAIKIN_ZERO_MARK;
        rmtdata[frameoffset].level0 = 1;            
        rmtdata[frameoffset].duration1 = DAIKIN_ZERO_TRAILER1;
        rmtdata[frameoffset].level1 = 0;
        frameoffset ++ ;

// create header signal

        rmtdata[frameoffset].duration0 = DAIKIN_HDR_MARK;
        rmtdata[frameoffset].level0 = 1;            
        rmtdata[frameoffset].duration1 = DAIKIN_HDR_SPACE;
        rmtdata[frameoffset].level1 = 0;
        frameoffset ++ ;

// create frame1 body

    int data2;
    for (int i = DAIKIN_FRAME1_OFFSET; i < DAIKIN_FRAME1_OFFSET + DAIKIN_FRAME1_LENGTH; i++) {
        data2=daikin[i];

        for (int j = 0; j < 8; j++) {
            rmtdata[frameoffset].duration0 = DAIKIN_ONE_MARK;
            rmtdata[frameoffset].level0 = 1;            

            if ((1 << j & data2)) {
            rmtdata[frameoffset].duration1 = DAIKIN_ONE_SPACE;
            }
            else 
            {
            rmtdata[frameoffset].duration1 = DAIKIN_ZERO_SPACE;
            }
            rmtdata[frameoffset].level1 = 0;
            frameoffset ++ ;
        }

    }

// create frame1 end

        rmtdata[frameoffset].duration0 = DAIKIN_ZERO_MARK;
        rmtdata[frameoffset].level0 = 1;            
        rmtdata[frameoffset].duration1 = DAIKIN_ZERO_TRAILER2;
        rmtdata[frameoffset].level1 = 0;
        frameoffset ++ ;

// frame 2

// create header signal

        rmtdata[frameoffset].duration0 = DAIKIN_HDR_MARK;
        rmtdata[frameoffset].level0 = 1;            
        rmtdata[frameoffset].duration1 = DAIKIN_HDR_SPACE;
        rmtdata[frameoffset].level1 = 0;
        frameoffset ++ ;

// create frame2 body

    for (int i = DAIKIN_FRAME2_OFFSET; i < DAIKIN_FRAME2_OFFSET + DAIKIN_FRAME2_LENGTH; i++) {
        data2=daikin[i];

        for (int j = 0; j < 8; j++) {
            rmtdata[frameoffset].duration0 = DAIKIN_ONE_MARK;
            rmtdata[frameoffset].level0 = 1;            

            if ((1 << j & data2)) {
            rmtdata[frameoffset].duration1 = DAIKIN_ONE_SPACE;
            }
            else 
            {
            rmtdata[frameoffset].duration1 = DAIKIN_ZERO_SPACE;
            }
            rmtdata[frameoffset].level1 = 0;
            frameoffset ++ ;
        }

    }

// create frame2 end

        rmtdata[frameoffset].duration0 = DAIKIN_ZERO_MARK;
        rmtdata[frameoffset].level0 = 1;            
        rmtdata[frameoffset].duration1 = DAIKIN_ZERO_SPACE;
        rmtdata[frameoffset].level1 = 0;
        frameoffset ++ ;

//send IR

    esp_err_t err = rmt_write_items(TX_CHANNEL, rmtdata, frameoffset, true);
    if (err != ESP_OK )
    {
        printf ("rmt_write_items failed%d\n", err);
        return;
    }

    rmt_wait_tx_done(TX_CHANNEL, portMAX_DELAY);

    printf("%d bits sent\n" , frameoffset);
    for (int i=0; i<=frameoffset; i++){
        printf( "%i %i ", rmtdata[i].duration0, rmtdata[i].duration1);
    }
    free(rmtdata);

    return;
}