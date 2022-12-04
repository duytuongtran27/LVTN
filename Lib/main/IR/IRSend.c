#include "IRSend.h"

void IRSendInstall(uint32_t freq, uint32_t duty_percent){
    IRDriverTXInstall(freq, duty_percent);
}
void IRSendUninstall(void){
    IRDriverTXUninstall();
}
void IRSendGeneric( const uint32_t headermark, const uint32_t headerspace,
                            const uint32_t onemark, const uint32_t onespace,
                            const uint32_t zeromark, const uint32_t zerospace,
                            const uint32_t footermark, const uint32_t gap,
                            const uint8_t *dataptr, const uint16_t nbytes,
                            const uint16_t frequency, const bool MSBfirst,
                            const uint8_t repeat, const uint8_t dutycycle)
    {
    // for (uint16_t r = 0; r <= repeat; r++)
    // {
        if (headermark && headerspace)
        {
            IRSendMarkSpace(headermark, headerspace);
        }

        // Data
        for (uint16_t i = 0; i < nbytes; i++)
        {
            IRSendData(onemark, onespace, zeromark, zerospace, *(dataptr + i), 8, MSBfirst);
        }
        // Footer
        // if (footermark && (gap > 0x7fff))
        // {
        //     IRSendMarkSpace(footermark, 0x7fff);
        // }
        // else if (footermark)
        // {
            IRSendMarkSpace(footermark, gap);
        // }
        // else
        // {
        if (headermark && headerspace)
        {
            IRSendMarkSpace(headermark, headerspace);
        }

        // Data
        for (uint16_t i = 0; i < nbytes; i++)
        {
            IRSendData(onemark, onespace, zeromark, zerospace, *(dataptr + i), 8, MSBfirst);
        }
            IRSendMarkSpace(0, 0);
        //}
    //}
}
void IRSendData(uint32_t onemark, uint32_t onespace, 
                uint32_t zeromark, uint32_t zerospace, 
                uint64_t data, uint16_t nbits,
                bool MSBfirst)
{
    if (nbits == 0) // If we are asked to send nothing, just return.
        return;
    if (MSBfirst)
    { 
        // Send the MSB first.
        // Send 0's until we get down to a bit size we can actually manage.
        while (nbits > sizeof(data) * 8)
        {
            IRSendMarkSpace(zeromark, zerospace);
            nbits--;
        }
        // Send the supplied data.
        for (uint64_t mask = 1ULL << (nbits - 1); mask; mask >>= 1)
        {
            if (data & mask)
            { 
                // Send a 1
                IRSendMarkSpace(onemark, onespace);
            }
            else
            { 
                // Send a 0
                IRSendMarkSpace(zeromark, zerospace);
            }
        }
    }
    else
    { 
        // Send the Least Significant Bit (LSB) first / MSB last.
        for (uint16_t bit = 0; bit < nbits; bit++, data >>= 1)
        {
            if (data & 1)
            { // Send a 1
                IRSendMarkSpace(onemark, onespace);
            }
            else
            { // Send a 0
                IRSendMarkSpace(zeromark, zerospace);
            }
        }
    }
}
void IRSendRAW(uint32_t *IRdata, int IRlength){
    IRDriverSendRAW(IRdata, IRlength);
}
void IRSendMarkSpace(uint32_t mask, uint32_t space){
    IRDriverSendMarkSpace(mask, space);
}
void IRSendMark(uint32_t mask){
    IRDriverSendMark(mask);
}
void IRSendSpace(uint32_t space){
    IRDriverSendSpace(space);
}