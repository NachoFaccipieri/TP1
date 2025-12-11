#include "aes.h"
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

// AES-128 S-box
static const uint8_t sbox[256] = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
};

static const uint8_t rcon[11] = {
    0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36
};

static void xor_block(uint8_t* dst, const uint8_t* src) {
    for (int i = 0; i < 16; i++) {
        dst[i] ^= src[i];
    }
}

static void key_expansion(const uint8_t* key, uint8_t* round_keys) {
    memcpy(round_keys, key, 16);
    
    for (int i = 1; i <= 10; i++) {
        uint8_t* prev = round_keys + (i - 1) * 16;
        uint8_t* curr = round_keys + i * 16;
        
        curr[0] = prev[0] ^ sbox[prev[13]] ^ rcon[i];
        curr[1] = prev[1] ^ sbox[prev[14]];
        curr[2] = prev[2] ^ sbox[prev[15]];
        curr[3] = prev[3] ^ sbox[prev[12]];
        
        for (int j = 4; j < 16; j++) {
            curr[j] = prev[j] ^ curr[j - 4];
        }
    }
}

static void sub_bytes(uint8_t* state) {
    for (int i = 0; i < 16; i++) {
        state[i] = sbox[state[i]];
    }
}

static void shift_rows(uint8_t* state) {
    uint8_t temp;
    
    temp = state[1];
    state[1] = state[5];
    state[5] = state[9];
    state[9] = state[13];
    state[13] = temp;
    
    temp = state[2];
    state[2] = state[10];
    state[10] = temp;
    temp = state[6];
    state[6] = state[14];
    state[14] = temp;
    
    temp = state[15];
    state[15] = state[11];
    state[11] = state[7];
    state[7] = state[3];
    state[3] = temp;
}

static uint8_t gf_mul(uint8_t a, uint8_t b) {
    uint8_t p = 0;
    for (int i = 0; i < 8; i++) {
        if (b & 1) p ^= a;
        uint8_t hi_bit = a & 0x80;
        a <<= 1;
        if (hi_bit) a ^= 0x1b;
        b >>= 1;
    }
    return p;
}

static void mix_columns(uint8_t* state) {
    for (int i = 0; i < 4; i++) {
        uint8_t* col = state + i * 4;
        uint8_t a[4];
        memcpy(a, col, 4);
        
        col[0] = gf_mul(a[0], 2) ^ gf_mul(a[1], 3) ^ a[2] ^ a[3];
        col[1] = a[0] ^ gf_mul(a[1], 2) ^ gf_mul(a[2], 3) ^ a[3];
        col[2] = a[0] ^ a[1] ^ gf_mul(a[2], 2) ^ gf_mul(a[3], 3);
        col[3] = gf_mul(a[0], 3) ^ a[1] ^ a[2] ^ gf_mul(a[3], 2);
    }
}

void aes_encrypt(const uint8_t* key, const uint8_t* input, uint8_t* output) {
    uint8_t round_keys[176];
    uint8_t state[16];
    
    key_expansion(key, round_keys);
    memcpy(state, input, 16);
    
    xor_block(state, round_keys);
    
    for (int round = 1; round < 10; round++) {
        sub_bytes(state);
        shift_rows(state);
        mix_columns(state);
        xor_block(state, round_keys + round * 16);
    }
    
    sub_bytes(state);
    shift_rows(state);
    xor_block(state, round_keys + 160);
    
    memcpy(output, state, 16);
}

static void left_shift(uint8_t* data) {
    uint8_t carry = 0;
    for (int i = 15; i >= 0; i--) {
        uint8_t new_carry = (data[i] & 0x80) >> 7;
        data[i] = (data[i] << 1) | carry;
        carry = new_carry;
    }
}

void aes_cmac(const uint8_t* key, const uint8_t* input, uint16_t length, uint8_t* output) {
    uint8_t zero[16] = {0};
    uint8_t L[16], K1[16], K2[16];
    uint8_t M[16] = {0};
    uint8_t X[16] = {0};
    
    // Generate subkeys
    aes_encrypt(key, zero, L);
    
    memcpy(K1, L, 16);
    left_shift(K1);
    if (L[0] & 0x80) K1[15] ^= 0x87;
    
    memcpy(K2, K1, 16);
    left_shift(K2);
    if (K1[0] & 0x80) K2[15] ^= 0x87;
    
    // Process blocks
    uint16_t n = (length + 15) / 16;
    if (n == 0) n = 1;
    
    bool complete = (length > 0) && (length % 16 == 0);
    
    for (uint16_t i = 0; i < n - 1; i++) {
        xor_block(X, input + i * 16);
        aes_encrypt(key, X, X);
    }
    
    // Last block
    uint16_t last_block_len = length - (n - 1) * 16;
    memcpy(M, input + (n - 1) * 16, last_block_len);
    
    if (complete) {
        xor_block(M, K1);
    } else {
        M[last_block_len] = 0x80;
        xor_block(M, K2);
    }
    
    xor_block(X, M);
    aes_encrypt(key, X, output);
}
