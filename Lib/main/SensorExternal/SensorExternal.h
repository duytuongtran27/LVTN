#ifndef __SensorExternal__
#define __SensorExternal__
#include "sht40.h"
#include "ltr303.h"
#include "scd40.h"
#ifdef __cplusplus
extern "C" {
#endif

void SensorExternalRun();

void sensor_task(void *arg);
void ltr303_task(void *arg);

#ifdef __cplusplus
}
#endif

#endif //__SensorExternal__