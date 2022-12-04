#ifndef DHT11_H_  
#define DHT11_H_

#include <stdbool.h>
#include <stdint.h>

#include "sdkconfig.h"

#define DHT11_PIN 13

typedef struct {
    uint8_t temperature; 
    uint8_t humidity;   
} dht11_data_t;

void dht11_init();

bool dht11_read(dht11_data_t *dht11_data);


#endif