#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/rmt.h"
#include "driver/periph_ctrl.h"
#include "soc/rmt_reg.h"
#include "freertos/event_groups.h"
#include "IRReceive.h"
#include <unistd.h>
#include "driver/rmt.h"

static const char *TAG = "IR Receive";

EventGroupHandle_t xEventGroup;
const rmt_channel_t RX_CHANNEL = RMT_CHANNEL_1;
const gpio_num_t RX_PIN = GPIO_NUM_14;			

void rmt_rx_init() {

	rmt_config_t rmt_rx_config;

	rmt_rx_config.rmt_mode      = RMT_MODE_RX;
	rmt_rx_config.channel       = RX_CHANNEL;
	rmt_rx_config.gpio_num      = RX_PIN;
	rmt_rx_config.clk_div       = 80;
	rmt_rx_config.mem_block_num = 4;

	rmt_rx_config.rx_config.filter_en           = false;
	rmt_rx_config.rx_config.filter_ticks_thresh = 0;
	rmt_rx_config.rx_config.idle_threshold      = 50000;

	rmt_config(&rmt_rx_config);
	esp_err_t err = rmt_driver_install(rmt_rx_config.channel, 4096, 0);
    if (err == ESP_OK){
        ESP_LOGI(TAG, "\ninstall rx successful");
    }
    else{
        ESP_LOGE(TAG, "rx error: ",err);
    }
}

void ir_rx_received() {
	size_t i;
    size_t rx_size = 0;
    rmt_item32_t* items = NULL;

    RingbufHandle_t rx_rb;

    // start receiving IR data

	if (rmt_rx_start(RMT_RX_CHANNEL, 1) == ESP_OK){
        ESP_LOGI(TAG, "start receive IR");
    }
    else{
        ESP_LOGE(TAG, "rx start error");
    }

	while (1) {
		// get the ring buffer handle
		rmt_get_ringbuf_handle(RMT_RX_CHANNEL, &rx_rb);

		// get items, if there are any
		items = (rmt_item32_t*) xRingbufferReceive(rx_rb, &rx_size, 100);
		if(items) {

			// print the RMT received durations to the monitor
            printf("\n");
			ESP_LOGI(TAG, "Received %i items", rx_size/4);
            for ( i=0; i<rx_size/4; i++ ) {
					if ( i>0 ) { printf(","); }
					printf( "%i", dur( items[i].level0, items[i].duration0 ));
					printf(",%i", dur( items[i].level1, items[i].duration1 ));
			}
			// free up data space
			vRingbufferReturnItem(rx_rb, (void*) items);
		}
	// delay 100 milliseconds
	vTaskDelay( 100 / portTICK_PERIOD_MS );
	}
    vTaskDelete(NULL);
}

int dur( uint32_t level, uint32_t duration ) {
	if ( level == 0 ) { return duration; }
	else { return -1.0 * duration; }
}
