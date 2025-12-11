#ifndef __AES_H__
#define __AES_H__

#include <stdint.h>

void aes_encrypt(const uint8_t* key, const uint8_t* input, uint8_t* output);
void aes_cmac(const uint8_t* key, const uint8_t* input, uint16_t length, uint8_t* output);

#endif // __AES_H__
