#include "Daikin.h"
#include "stdio.h"

static void checksum()
{
	uint8_t sum = 0;
	uint8_t i;


	for(i = 0; i <= 18; i++){
		sum += daikin[i];
	}

    daikin[19] = sum &0xFF;

    sum=0;
	for(i = 20; i <= 37; i++){
		sum += daikin[i];
    }

    daikin[38] = sum &0xFF;
}
static void on()
{
	daikin[25] |= 0x01;
    switch (daikin[25] >> 4)
    {
        case 2: //dry
            daikin[9] = vprocessstatus[7];
        break;
        case 3: //cool
            daikin[9] = vprocessstatus[6];
        break;
        case 4: //warm
            daikin[9] = vprocessstatus[8];
        break;
        case 6: //fan
            daikin[9] = vprocessstatus[11];
        break;
    }
    daikin[11] =  0x00;
	checksum();
}

static void off()
{
	daikin[25] &= 0xFE;
    daikin[9] = vprocessstatus[0];
    daikin[11] =  0x80;
	checksum();
}

void Daikin_setPower(uint8_t state)
{
	if (state == 0) {
		off();
	}else {
		on();
	}
}

static void setSwing_on()
{
	daikin[28] |=0x0f;
	checksum();
}

static void setSwing_off()
{
	daikin[28] &=0xf0;
	checksum();
}

void Daikin_setSwing(uint8_t state)
{
	if (state == 0) {
		setSwing_off();
	}else {
		setSwing_on();
	}
}

static void setSwingLR_on()
{
	daikin[29] = daikin[29] | 0x0F;
	checksum();
}

static void setSwingLR_off()
{
	daikin[29] = daikin[29] & 0xF0;
	checksum();
}

void Daikin_setSwingLR(uint8_t state)
{
    if (state == 0) {
        setSwingLR_off();
	}else {
        setSwingLR_on();
	}
}

uint8_t getPower()
{
	return (daikin[25] & 0x01);
}

void Daikin_setMode(uint8_t mode)
{
	uint8_t trmode = vModeTable[mode];
	if (mode>=0 && mode <=4)
	{
		daikin[25] = ((daikin[25] & 0x0f) | trmode << 4) | getPower();
		checksum();
	}
}

// 0~4 speed,5 auto,6 moon
void Daikin_setFan(uint8_t speed)
{
	uint8_t fan = vFanTable[speed];
	if (speed>=0 && speed <=6)
	{
		daikin[28] &= 0x0f;
		daikin[28] |= fan;
		checksum();
	}
}

void Daikin_setTemp(uint8_t temp)
{
	if (temp >= 18 && temp<=32)
	{
		daikin[26] = (temp)*2;
		checksum();
	}
}