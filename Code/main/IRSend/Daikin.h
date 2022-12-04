#ifndef _daikin_h_
#define _daikin_h_

#include "stdio.h"

const int wakedatalength = 5;
const int headerlength = 1;
const int trailerlength = 1;
const int frame1bytelength = 20;
const int frame2bytelength = 19;
const int endframelength = 1;

#define CARRIER_FREQ            38000
#define DUTY_PERCENT            33

#define DAIKIN_HDR_MARK	        3600
#define DAIKIN_HDR_SPACE        1880
#define DAIKIN_ONE_MARK	        348
#define DAIKIN_ONE_SPACE	1386
#define DAIKIN_ZERO_MARK        348
#define DAIKIN_ZERO_SPACE       490
#define DAIKIN_ZERO_TRAILER1    25000
#define DAIKIN_ZERO_TRAILER2    30000

unsigned char daikin[]     = {
        0x11, 0xDA, 0x27, 0x00, 0x02, 0x00, 0x00, 0x00,
//        0     1     2    3     4     5      6    7
        0x00, 0x0e, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00,
//        8     9    10    11    12     13    14    15 
        0x00, 0x00, 0x00, 0x00, 0x11, 0xDA, 0x27, 0x00, 
//        16    17    18    19    20    21    22    23 
        0x00, 0x39, 0xB0, 0x00, 0x00, 0x00, 0x00, 0x06, 
//        24    25    26    27    28    29    30    31 
        0x60, 0x00, 0x00, 0xc3, 0x00, 0x00, 0x00
//        32    33    34    35    36    37    38
		};

#define DAIKIN_FRAME1_LENGTH 20 
#define DAIKIN_FRAME1_OFFSET 0
#define DAIKIN_FRAME2_LENGTH 19
#define DAIKIN_FRAME2_OFFSET DAIKIN_FRAME1_LENGTH

unsigned char vFanTable[7] = { 0x30,0x40,0x50,0x60,0x70,0xa0,0xb0};
//                        0 FAN 1 COOL 2 DRY 3 HEAT
unsigned char vModeTable[5] = { 0x6,0x3,0x2,0x4,0x00};

unsigned char vprocessstatus[18] = {
        0x02, //stop                                    0
        0x03, //change temp.                            1   
        0x06, //change wind direction                   2
        0x07, //change wind power                       3       
        0x0c, //dismiss timer setting                   4
        0x0d, //comfort auto                            5
        0x0e, //cool                                    6
        0x0f, //dry                                     7
        0x10, //warm                                    8
        0x16, //change wind direction horizontally      9   
        0x18, //clean inside the aircon                 10  
        0x1a, //fan                                     11
        0x1c, //laundry mode                            12
        0x1e, //change the status for on timer          13
        0x1f, //change the status for off timer         14
        0x2f, //set a sleep timer                       15
        0x34, //set "kaze-nai-su" mode                  16
        0x35  //set strema                              17
};

    void Daikin_setPower(uint8_t state);
    void Daikin_setSwing(uint8_t state);
    void Daikin_setSwingLR(uint8_t state);
    void Daikin_setMode(uint8_t mode);
    void Daikin_setFan(uint8_t speed);
    void Daikin_setTemp(uint8_t temp);
#endif