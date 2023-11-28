#include <stdint.h> // for uint16_t

void
inst_to_bits (char *dest, uint16_t inst)
{
  dest[0x0F] = (inst & 0x0001) ? '1' : '0';
  dest[0x0E] = (inst & 0x0002) ? '1' : '0';
  dest[0x0D] = (inst & 0x0004) ? '1' : '0';
  dest[0x0C] = (inst & 0x0008) ? '1' : '0';
  dest[0x0B] = (inst & 0x0010) ? '1' : '0';
  dest[0x0A] = (inst & 0x0020) ? '1' : '0';
  dest[0x09] = (inst & 0x0040) ? '1' : '0';
  dest[0x08] = (inst & 0x0080) ? '1' : '0';
  dest[0x07] = (inst & 0x0100) ? '1' : '0';
  dest[0x06] = (inst & 0x0200) ? '1' : '0';
  dest[0x05] = (inst & 0x0400) ? '1' : '0';
  dest[0x04] = (inst & 0x0800) ? '1' : '0';
  dest[0x03] = (inst & 0x1000) ? '1' : '0';
  dest[0x02] = (inst & 0x2000) ? '1' : '0';
  dest[0x01] = (inst & 0x4000) ? '1' : '0';
  dest[0x00] = (inst & 0x8000) ? '1' : '0';
  dest[0x10] = 0;
}
