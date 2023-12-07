#pragma once

#include <stdint.h> // for uint16_t

#define SWAP16(x) ((x << 8) | (x >> 8))
#define SIGN_EXT(x, bits)                                                     \
  ((((x) >> ((bits)-1)) & 1) ? ((x) | (0xFFFF << (bits))) : (x))

void inst_to_bits (char *dest, uint16_t inst);
const char *unescape_string (char *dest, const char *str);
