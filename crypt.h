#ifndef SHA1_DEF
#define SHA1_DEF
#include <stdint.h>
void SHA1GenerateHash();
void SHA1PadMessage(char *msg, uint64_t length);
void SHA1(char *msg, uint64_t length,uint32_t *res);
void base64_encode(uint8_t *str, int size, char *enc);

#endif