/*
 * IRDriver.h
 *
 *  Created on: May 15, 2022
 *      Author: NhatHoang
 */
#ifndef __IRDRIVER__
#define __IRDRIVER__

#include "driver/rmt.h"
#include "esp_log.h"
#include "hw_config.h"
#include "piezo_driver.h"

#define IR_GPIO_RX                      IR_RX_PIN
#define IR_GPIO_RAW                     IR_RAW_PIN

#define IR_GPIO_TX                      IR_TX_PIN

#define CHANNEL_IR_RMT_TX			    RMT_CHANNEL_0

#define CHANNEL_IR_RMT_RX			    RMT_CHANNEL_2

#define CLK_DIV                         80
#define TICK_10_US                      (80000000 / CLK_DIV / 100000)

#define roundTo                         50 
#define MARK_EXCESS                     100 
#define SPACE_EXCESS                    100 

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    IR_STATE_IDLE = 0,
    IR_STATE_TX,
    IR_STATE_RX,
    IR_STATE_RAW,
    IR_STATE_ISR,
    IR_STATE_UNKNOWN = 255,
} IRState_t;

typedef struct {
    uint32_t lastChange;
    uint32_t spaceLen;
    uint32_t markLen;
    uint32_t markStart;
    uint32_t spaceStart;
    uint16_t pulseCount;
    uint32_t space[512];
    uint32_t mark[512];
    uint32_t pulse[512];
}IRRAWData_t;

typedef struct 
{
   uint32_t MarkHeader;
   uint32_t SpaceHeader;
   uint32_t MarkZero;
   uint32_t SpaceZero;
   uint32_t MarkOne;
   uint32_t SpaceOne;
   uint32_t Gap;
   uint32_t Repeat;
   uint32_t Tolerance;
}IRParamater_t;

IRState_t IRDriverGetState(void);
void IRDriverSetStartCondition(uint32_t Mark, uint32_t Space, uint32_t Tolerance);

void IRDriverRAWInstall(void);
void IRDriverRAWUninstall(void);

void IRDriverRAWCreateTask(void);
void IRDriverRAWDeleteTask(void);

void IRDriverRXInstall(void);
void IRDriverRXUninstall(void);
void IRDriverRXStart(void); 
void IRDriverRXStop(void);

void IRDriverRXCreateTask(void);
void IRDriverRXDeleteTask(void);

void IRDriverTXInstall(uint32_t freq, uint32_t duty_percent);
void IRDriverTXUninstall(void);

void IRDriverSendRAW(uint32_t *IRdata, uint16_t IRlength);
void IRDriverSendMarkSpace(uint32_t mask, uint32_t space);
void IRDriverSendMark(uint32_t mask);
void IRDriverSendSpace(uint32_t space);

char* IRDriverGetCaptureStream(void);

#ifdef __cplusplus
}
#endif

#endif /* __IRDRIVER__*/
