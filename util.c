#include "util.h"
#include "lc3.h"
#include <stdio.h>

void
inst_to_bits (char *dest, uint16_t inst)
{
  char *p = dest;
  for (int i = 15; i >= 0; i--)
    {
      if (inst & (1 << i))
        *p++ = '1';
      else
        *p++ = '0';

      if (i && i % 4 == 0)
        *p++ = ' ';
    }

  // null-terminate like a good citizen
  *p = 0;
}

uint16_t
sign_extend (uint16_t x, int bit_count)
{
  if ((x >> (bit_count - 1)) & 1)
    {
      x |= (0xFFFF << bit_count);
    }
  return x;
}

int
unescape_string (char *dest, const char *str)
{
  int err_count = 0;
  char *d = dest;
  for (const char *p = str; *p; p++)
    {
      if (*p == '\\')
        {
          switch (*(++p))
            {
              // clang-format off
            case 'a':  *d++ = '\007';  break;
            case 'b':  *d++ = '\b';    break;
            case 'e':  *d++ = '\e';    break;
            case 'f':  *d++ = '\f';    break;
            case 'n':  *d++ = '\n';    break;
            case 'r':  *d++ = '\r';    break;
            case 't':  *d++ = '\t';    break;
            case 'v':  *d++ = '\013';  break;
            case '\\': *d++ = '\\';    break;
            case '?':  *d++ = '?';     break;
            case '\'': *d++ = '\'';    break;
            case '"':  *d++ = '\"';    break;
            default:
              {
                fprintf (stderr,
                         "warning: I don't understand escape sequence \\%c\n",
                         *p);
                err_count++;
              }
              // clang-format on
            }
        }
      else
        {
          *d++ = *p;
        }
    }
  *d = 0;
  return err_count;
}
