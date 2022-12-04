#ifndef __IRSEND__
#define __IRSEND__
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "IRDriver.h"
#ifdef __cplusplus
extern "C" {
#endif

void IRSendInstall(uint32_t freq, uint32_t duty_percent);
void IRSendUninstall(void);
void IRSendGeneric( const uint32_t headermark, const uint32_t headerspace,
                    const uint32_t onemark, const uint32_t onespace,
                    const uint32_t zeromark, const uint32_t zerospace,
                    const uint32_t footermark, const uint32_t gap,
                    const uint8_t *dataptr, const uint16_t nbytes,
                    const uint16_t frequency, const bool MSBfirst,
                    const uint8_t repeat, const uint8_t dutycycle);
void IRSendData(uint32_t onemark, uint32_t onespace, 
                uint32_t zeromark, uint32_t zerospace, 
                uint64_t data, uint16_t nbits,
                bool MSBfirst);
void IRSendRAW(uint32_t *IRdata, int IRlength);
void IRSendMarkSpace(uint32_t mask, uint32_t space);
void IRSendMark(uint32_t mask);
void IRSendSpace(uint32_t space);

#ifdef __cplusplus
}
#endif
#endif //__IRSEND__