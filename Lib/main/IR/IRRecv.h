#ifndef __IRRECV__
#define __IRRECV__

#include "IRDriver.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void IRRecvRXCreateTask(void);
void IRRecvRXDeleteTask(void);
void IRRecvRAWCreateTask(void);
void IRRecvRAWDeleteTask(void);
void IRRecvSetStartCondition(uint32_t Mark, uint32_t Space, uint32_t Tolerance);
char* IRRecvGetCaptureStream(void);

#ifdef __cplusplus
}
#endif
#endif //__IRRECV__