#include "driver/rmt.h"

#ifndef _IR_RECEIVE_
#define _IR_RECEIVE_

#define RMT_RX_CHANNEL RMT_CHANNEL_1

    void rmt_rx_init();
    void ir_rx_received();
    int dur( uint32_t level, uint32_t duration );
#endif