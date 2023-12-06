#pragma once

#include <stdint.h> // for uint16_t

#define SWAP16(x) ((x << 8) | (x >> 8))

void inst_to_bits (char *dest, uint16_t inst);
uint16_t sign_extend (uint16_t x, int bit_count);
int unescape_string(char *dest, const char *str);
