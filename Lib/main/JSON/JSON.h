#ifndef __JSON__
#define __JSON__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void JSON_Deserialize(const char *value);
uint8_t  devsGetCmd(char *text);
uint8_t  devsParseCmd(char *text);

#ifdef __cplusplus
}
#endif

#endif //__JSON__